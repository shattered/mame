// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*** t11: Portable DEC T-11 emulator ******************************************

    System dependencies:    long must be at least 32 bits
                            word must be 16 bit unsigned int
                            byte must be 8 bit unsigned int
                            long must be more than 16 bits
                            arrays up to 65536 bytes must be supported
                            machine must be twos complement

*****************************************************************************/

#include "emu.h"
#include "debugger.h"
#include "t11.h"


/*************************************
 *
 *  Macro shortcuts
 *
 *************************************/

/* registers of various sizes */
#define REGD(x) m_reg[x].d
#define REGW(x) m_reg[x].w.l
#define REGB(x) m_reg[x].b.l

/* PC, SP, and PSW definitions */
#define SP      REGW(6)
#define PC      REGW(7)
#define SPD     REGD(6)
#define PCD     REGD(7)
#define PSW     m_psw.w.l
#define CPC     m_cpc.w.l
#define CPSW    m_cpsw.w.l

/* HALT mode */
#define SET_CPC(x) \
	do { PC = (x); if((PSW & 0600) != 0600) CPC = PC; } while (0)
#define SET_CPSW(x,t) \
	do { PSW = (t) ? (x) : (((x) & ~TFLAG) | (PSW & TFLAG)); if((PSW & 0600) != 0600) CPSW = PSW; } while (0)

const device_type T11 = &device_creator<t11_device>;
const device_type K1801VM2 = &device_creator<k1801vm2_device>;


k1801vm2_device::k1801vm2_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: t11_device(mconfig, K1801VM2, "K1801VM2", tag, owner, clock, "k1801vm2", __FILE__)
	, m_dipswitch_func(*this)
{
}

t11_device::t11_device(const machine_config &mconfig, device_type type, const char *name, const char *tag, device_t *owner, UINT32 clock, const char *shortname, const char *source)
	: cpu_device(mconfig, type, name, tag, owner, clock, shortname, source)
	, z80_daisy_chain_interface(mconfig, *this)
	, m_program_config("program", ENDIANNESS_LITTLE, 16, 16, 0)
	, c_initial_mode(0)
	, c_features(0)
	, m_bankswitch_func(*this)
	, m_out_reset_func(*this)
{
	m_program_config.m_is_octal = true;
	memset(m_reg, 0x00, sizeof(m_reg));
	memset(&m_psw, 0x00, sizeof(m_psw));
}

t11_device::t11_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock)
	: cpu_device(mconfig, T11, "T11", tag, owner, clock, "t11", __FILE__)
	, z80_daisy_chain_interface(mconfig, *this)
	, m_program_config("program", ENDIANNESS_LITTLE, 16, 16, 0)
	, c_initial_mode(0)
	, c_features(0)
	, m_bankswitch_func(*this)
	, m_out_reset_func(*this)
{
	m_program_config.m_is_octal = true;
	memset(m_reg, 0x00, sizeof(m_reg));
	memset(&m_psw, 0x00, sizeof(m_psw));
}


/*************************************
 *
 *  Low-level memory operations
 *
 *************************************/

int t11_device::ROPCODE(int how)
{
	if (m_disabled)
		device_reset();

	PC &= 0xfffe;
	int val = m_direct->read_word(PC);
	if (0) {
		logerror("VM2: ROPCODE(%d) at %06o, cpc %06o, psw %03o\n", how, PC, CPC, PSW);
	}
	if (!m_bus_error) {
		SET_CPC(PC + 2);
	}
	return val;
}


int t11_device::RBYTE(int addr)
{
	return m_program->read_byte(addr);
}


void t11_device::WBYTE(int addr, int data)
{
	m_program->write_byte(addr, data);
}


int t11_device::RWORD(int addr)
{
	return m_program->read_word(addr & 0xfffe);
}


void t11_device::WWORD(int addr, int data)
{
	m_program->write_word(addr & 0xfffe, data);
}



/*************************************
 *
 *  Low-level stack operations
 *
 *************************************/

void t11_device::PUSH(int val)
{
	SP -= 2;
	WWORD(SPD, val);
}


int t11_device::POP()
{
	int result = RWORD(SPD);
	SP += 2;
	return result;
}



/*************************************
 *
 *  Flag definitions and operations
 *
 *************************************/

/* flag definitions */
#define CFLAG 0001
#define VFLAG 0002
#define ZFLAG 0004
#define NFLAG 0010
#define TFLAG 0020
#define PFLAG 0200
#define HFLAG 0400

/* extracts flags */
#define GET_C (PSW & CFLAG)
#define GET_V (PSW & VFLAG)
#define GET_Z (PSW & ZFLAG)
#define GET_N (PSW & NFLAG)
#define GET_T (PSW & TFLAG)
#define GET_P (PSW & PFLAG)
#define GET_H (PSW & HFLAG)

/* clears flags */
#define CLR_C (PSW &= ~CFLAG)
#define CLR_V (PSW &= ~VFLAG)
#define CLR_Z (PSW &= ~ZFLAG)
#define CLR_N (PSW &= ~NFLAG)
#define CLR_T (PSW &= ~TFLAG)
#define CLR_P (PSW &= ~PFLAG)
#define CLR_H (PSW &= ~HFLAG)

/* sets flags */
#define SET_C (PSW |= CFLAG)
#define SET_V (PSW |= VFLAG)
#define SET_Z (PSW |= ZFLAG)
#define SET_N (PSW |= NFLAG)
#define SET_T (PSW |= TFLAG)
#define SET_H (PSW |= HFLAG)



/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

struct irq_table_entry
{
	UINT8   priority;
	UINT8   vector;
};

static const struct irq_table_entry irq_table[] =
{
	{ 0<<5, 0x00 },
	{ 4<<5, 0x38 },
	{ 4<<5, 0x34 },
	{ 4<<5, 0x30 },
	{ 5<<5, 0x5c },
	{ 5<<5, 0x58 },
	{ 5<<5, 0x54 },
	{ 5<<5, 0x50 },
	{ 6<<5, 0x4c },
	{ 6<<5, 0x48 },
	{ 6<<5, 0x44 },
	{ 6<<5, 0x40 },
	{ 7<<5, 0x6c },
	{ 7<<5, 0x68 },
	{ 7<<5, 0x64 },
	{ 7<<5, 0x60 }
};

void t11_device::t11_check_irqs()
{
	const struct irq_table_entry *irq = &irq_table[m_irq_state & 15];
	int priority = PSW & 0xe0;

	/* compare the priority of the interrupt to the PSW */
	if (irq->priority > priority)
	{
		int vector = irq->vector;
		int new_pc, new_psw;

		/* call the callback; if we don't get -1 back, use the return value as our vector */
		int new_vector = standard_irq_callback(m_irq_state & 15);
		if (new_vector != -1)
			vector = new_vector;

		/* fetch the new PC and PSW from that vector */
		assert((vector & 3) == 0);
		new_pc = RWORD(vector);
		new_psw = RWORD(vector + 2);

		/* push the old state, set the new one */
		PUSH(PSW);
		PUSH(PC);
		PCD = new_pc;
		PSW = new_psw;
		t11_check_irqs();

		/* count cycles and clear the WAIT flag */
		m_icount -= 114;
		m_wait_state = 0;
	}
}

/*
 * interrupt priorities.  1 and 2 are nonmaskable and are handled elsewhere.
 * machine and device drivers can assert ACLO, HALT, EVNT and VIRQ pins.
 *
 * 1 - bus errors
 * 2 - illegal instruction or reserved operand
 * 3 - trace trap.  masked by m_wait_state.
 * 4 - ACLO pin (power fail).  masked by pflag AND hflag.
 * 5 - HALT pin.  masked by hflag.
 * 6 - EVNT pin (line clock).  masked by pflag.
 * 7 - VIRQ pin (vectored interrupt request).  masked by pflag.
 *
 * internal states:
 *
 * m_bus_error
 * m_aclo_state -- cleared on vector fetch
 * m_evnt_state -- cleared by RESET insn and on vector fetch.
 * m_wait_state
 *
 */
void k1801vm2_device::t11_check_irqs()
{
	int new_pc, new_psw, new_vector;

#if 0
	logerror("%11.6f at %s: "
		"VM2: check_irqs: pc %06o psw %03o irqs %03o evnt %d wait %d\n", 
		machine().time().as_double(),machine().describe_context(),
		PC, PSW, m_irq_state, m_evnt_state, m_wait_state);
#endif

	if (GET_T) {
		t11_device::_trap(T11_BPT, 0, 0);
		return;
	}

	if (!GET_H && BIT(m_irq_state, INPUT_LINE_1801HALT)) {
		t11_device::halt(0);
		return;
	}

	if (GET_P) {
		return;
	}

	if (!GET_H && BIT(m_irq_state, INPUT_LINE_ACLO)) {
		t11_device::_trap(T11_PWRFAIL, 0, 0);
		execute_set_input(INPUT_LINE_ACLO, CLEAR_LINE);
		return;
	}

	if (m_evnt_state) {
		t11_device::_trap(T11_EVNT, 0, 0);
		m_evnt_state = FALSE;
		return;
	}

	if (!BIT(m_irq_state, INPUT_LINE_VIRQ))
		return;

	/* call the callback; if we don't get -1 back, use the return value as our vector */
	new_vector = daisy_call_ack_device();
	if (new_vector == -1 || new_vector == 0) {
		execute_set_input(INPUT_LINE_VIRQ, CLEAR_LINE);
//		t11_device::_trap(0274, 1, 0);
		return;
	}

	/* fetch the new PC and PSW from that vector */
	assert((new_vector & 3) == 0);

	CLR_H;
	m_bankswitch_func(0);
	new_pc = RWORD(new_vector); IF_BUSERR(return);
	new_psw = RWORD(new_vector + 2); IF_BUSERR(return);

	m_evnt_state = FALSE;

	PUSH(PSW); IF_BUSERR(return);
	PUSH(PC); IF_BUSERR(return);
	PCD = new_pc;
	PSW = new_psw;

	t11_check_irqs();

	/* count cycles and clear the WAIT flag */
	m_icount -= 114;
	m_wait_state = 0;
}


/*************************************
 *
 *  Core opcodes
 *
 *************************************/

/* includes the static function prototypes and the master opcode table */
#include "t11table.hxx"

/* includes the actual opcode implementations */
#include "t11ops.hxx"



/*************************************
 *
 *  Low-level initialization/cleanup
 *
 *************************************/

void t11_device::device_start()
{
	static const UINT16 initial_pc[] =
	{
		0xc000, 0x8000, 0x4000, 0x2000,
		0x1000, 0x0000, 0xf600, 0xf400
	};

	m_initial_pc = initial_pc[c_initial_mode >> 13];
	m_program = &space(AS_PROGRAM);
	m_direct = &m_program->direct();
	m_out_reset_func.resolve_safe();

	save_item(NAME(m_ppc.w.l));
	save_item(NAME(m_reg[0].w.l));
	save_item(NAME(m_reg[1].w.l));
	save_item(NAME(m_reg[2].w.l));
	save_item(NAME(m_reg[3].w.l));
	save_item(NAME(m_reg[4].w.l));
	save_item(NAME(m_reg[5].w.l));
	save_item(NAME(m_reg[6].w.l));
	save_item(NAME(m_reg[7].w.l));
	save_item(NAME(m_psw.w.l));
	save_item(NAME(m_initial_pc));
	save_item(NAME(m_wait_state));
	save_item(NAME(m_evnt_state));
	save_item(NAME(m_irq_state));
	save_item(NAME(m_bus_error));

	// Register debugger state
	state_add( T11_PC,  "PC",  m_reg[7].w.l).formatstr("%06O");
	state_add( T11_SP,  "SP",  m_reg[6].w.l).formatstr("%06O");
	state_add( T11_PSW, "PSW", m_psw.w.l).formatstr("%06O");
	state_add( T11_R0,  "R0",  m_reg[0].w.l).formatstr("%06O");
	state_add( T11_R1,  "R1",  m_reg[1].w.l).formatstr("%06O");
	state_add( T11_R2,  "R2",  m_reg[2].w.l).formatstr("%06O");
	state_add( T11_R3,  "R3",  m_reg[3].w.l).formatstr("%06O");
	state_add( T11_R4,  "R4",  m_reg[4].w.l).formatstr("%06O");
	state_add( T11_R5,  "R5",  m_reg[5].w.l).formatstr("%06O");
	state_add( T11_CPC, "CPC", m_cpc.w.l).formatstr("%06O");
	state_add( T11_CPSW,"CPSW",m_cpsw.w.l).formatstr("%06O");

	state_add(STATE_GENPC, "GENPC", m_reg[7].w.l).noshow();
	state_add(STATE_GENPCBASE, "CURPC", m_ppc.w.l).noshow();
	state_add(STATE_GENFLAGS, "GENFLAGS", m_psw.w.l).formatstr("%8s").noshow();

	m_icountptr = &m_icount;
	m_bankswitch_func.resolve_safe();
}

void k1801vm2_device::device_start()
{
	t11_device::device_start();

	m_dipswitch_func.resolve_safe(0);

	logerror("CPU: device_start()\n");
}

void t11_device::state_string_export(const device_state_entry &entry, std::string &str) const
{
	switch (entry.index())
	{
		case STATE_GENFLAGS:
			str = string_format("%c%c%c%c%c%c%c%c",
				m_psw.b.l & 0x80 ? 'I':'.',
				m_psw.b.l & 0x40 ? 'I':'.',
				m_psw.b.l & 0x20 ? 'I':'.',
				m_psw.b.l & 0x10 ? 'T':'.',
				m_psw.b.l & 0x08 ? 'N':'.',
				m_psw.b.l & 0x04 ? 'Z':'.',
				m_psw.b.l & 0x02 ? 'V':'.',
				m_psw.b.l & 0x01 ? 'C':'.'
			);
			break;
	}
}

void k1801vm2_device::state_string_export(const device_state_entry &entry, std::string &str) const
{
	switch (entry.index())
	{
		case STATE_GENFLAGS:
			str = string_format("%c%c%c%c%c%c%c%c%c",
				m_psw.b.l & HFLAG? 'H':'.',
				m_psw.b.l & 0x80 ? 'I':'.',
				m_psw.b.l & 0x40 ? 'I':'.',
				m_psw.b.l & 0x20 ? 'I':'.',
				m_psw.b.l & TFLAG? 'T':'.',
				m_psw.b.l & NFLAG? 'N':'.',
				m_psw.b.l & ZFLAG? 'Z':'.',
				m_psw.b.l & VFLAG? 'V':'.',
				m_psw.b.l & CFLAG? 'C':'.'
			);
			break;
	}
}


/*************************************
 *
 *  CPU reset
 *
 *************************************/

void t11_device::device_reset()
{
	/* initial SP is 376 octal, or 0xfe */
	SP = 0x00fe;

	/* initial PC comes from the setup word */
	PC = m_initial_pc;

	/* PSW starts off at highest priority */
	PSW = 0xe0;

	/* initialize the IRQ state */
	m_irq_state = 0;

	/* reset the remaining state */
	REGD(0) = 0;
	REGD(1) = 0;
	REGD(2) = 0;
	REGD(3) = 0;
	REGD(4) = 0;
	REGD(5) = 0;
	m_ppc.d = 0;
	m_wait_state = 0;
	m_evnt_state = FALSE;
	m_bus_error = 0;
	m_disabled = suspended(SUSPEND_REASON_DISABLE);
}

void k1801vm2_device::device_reset()
{
	if (m_disabled & !suspended(SUSPEND_REASON_DISABLE)) {
		logerror("CPU: device_reset(), resetting after resume\n");
		m_disabled = suspended(SUSPEND_REASON_DISABLE);
	}

	t11_device::device_reset();

	// start in HALT mode
	m_bankswitch_func(0);
	m_bankswitch_func(1);

	c_initial_mode |= (m_dipswitch_func(0) & 7);

	CPC  = PC  = RWORD (c_initial_mode & 0xff00);
	CPSW = PSW = RWORD((c_initial_mode & 0xff00)+2);

	logerror("CPU: device_reset(), PC %06o PSW %06o\n", PC, PSW);

	m_bankswitch_func(BIT(PSW, 8));
}


/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

void t11_device::execute_set_input(int irqline, int state)
{
	/* set the appropriate bit */
	if (state == CLEAR_LINE)
		m_irq_state &= ~(1 << irqline);
	else
		m_irq_state |= 1 << irqline;
}

void k1801vm2_device::execute_set_input(int irqline, int state)
{
	if (irqline == INPUT_LINE_BUSERR) {
		if (state) {
		m_bus_error++;
//		logerror("VM2: bus error (->%d) in %s mode (assert)\n", m_bus_error, BIT(PSW,8)?"HALT":"USER");
		}
		return;
	}

	if (irqline == INPUT_LINE_EVNT) {
		m_evnt_state = state;
		return;
	}

	/* set the appropriate bit */
	if (state == CLEAR_LINE)
		m_irq_state &= ~(1 << irqline);
	else
		m_irq_state |= 1 << irqline;
}



/*************************************
 *
 *  Core execution
 *
 *************************************/

// page 1-5 of T11_UsersMan

void t11_device::execute_run()
{
	t11_check_irqs();

	if (m_wait_state)
	{
		m_icount = 0;
		return;
	}

	do
	{
		UINT16 op;

		m_ppc = m_reg[7];   /* copy PC to previous PC */

		debugger_instruction_hook(this, PCD);

		op = ROPCODE(0);
		(this->*s_opcode_table[op >> 3])(op);

	} while (m_icount > 0);
}

// page XXX of XXX

void k1801vm2_device::execute_run()
{
	t11_check_irqs();

	if (m_wait_state)
	{
		m_icount = 0;
		return;
	}

	do
	{
		UINT16 op;

		m_ppc = m_reg[7];   /* copy PC to previous PC */

		debugger_instruction_hook(this, PCD);

//		logerror("VM2: insn decode 1 (%06o, %s, be %d)\n", PC, BIT(PSW,8)?"HALT":"USER", m_bus_error);
		op = ROPCODE(0); 
//		logerror("VM2: insn decode 2 (%06o, %s, be %d, op %06o)\n", PC, BIT(PSW,8)?"HALT":"USER", m_bus_error, op);
		if (m_bus_error) {
			logerror("VM2: bus error (%d) in %s mode at %06o (insn decode)\n", m_bus_error, BIT(PSW,8)?"HALT":"USER", PC);
			bus_error(0);
		} else {
			m_bus_error = 0;
			(this->*s_opcode_table[op >> 3])(op);
		}

	} while (m_icount > 0);
}



offs_t t11_device::disasm_disassemble(char *buffer, offs_t pc, const UINT8 *oprom, const UINT8 *opram, UINT32 options)
{
	extern CPU_DISASSEMBLE( t11 );
	return CPU_DISASSEMBLE_NAME(t11)(this, buffer, pc, oprom, opram, options);
}
