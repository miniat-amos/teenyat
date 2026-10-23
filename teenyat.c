/*
 * Name	   : teenyat.c
 * Author	 : William "Amos" Confer
 *
 * License	: Copyright (C) 2023 All rights reserved
 */

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "teenyat.h"

/*
 * Platform Independent microsecond clock function
 */
#if defined(_WIN64) || defined(_WIN32)
	#include <windows.h>
	/* Query windows high resolution clock for time */
	uint64_t us_clock(void) {
		LARGE_INTEGER frequency, counter;
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&counter);
		/* Convert to micorseconds */
		return (counter.QuadPart * 1000000LL) / frequency.QuadPart;
	}
#else
	#include <unistd.h>
	uint64_t us_clock(void) {
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		/* Convert to micorseconds */
		return (uint64_t)(ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL);
	}
#endif

tny_uword tny_random(teenyat *t);
uint64_t  tny_calibrate_1_us(void);

static void set_elg_flags(teenyat *t, tny_sword alu_result) {
	t->flags.equals  = (alu_result == 0);
	t->flags.less    = (alu_result >> 15) & 1;
	t->flags.greater = (alu_result > 0);

	return;
}

static inline void trunc_pc(teenyat *t) {
	assert(t != NULL);
	t->reg[TNY_REG_PC].u &= TNY_MAX_RAM_ADDRESS;

	return;
}

static inline void set_pc(teenyat *t, tny_uword addr) {
	assert(t != NULL);
	t->reg[TNY_REG_PC].u = addr;
	trunc_pc(t);

	return;
}

static inline void inc_pc(teenyat *t) {
	set_pc(t, t->reg[TNY_REG_PC].u + 1);

	return;
}

static inline void dec_pc(teenyat *t) {
	set_pc(t, t->reg[TNY_REG_PC].u - 1);

	return;
}

static void default_bus_read(teenyat *t, tny_uword addr, tny_word *data, uint16_t *delay) {
	/* all parameters unused */
	(void)t;
	(void)addr;
	(void)data;
	(void)delay;

	return;
}

static void default_bus_write(teenyat *t, tny_uword addr, tny_word data, uint16_t *delay) {
	/* all parameters unused */
	(void)t;
	(void)addr;
	(void)data;
	(void)delay;

	return;
}

bool tny_init_from_file(teenyat *t, FILE *bin_file) {

	if(!t) {
		return false;
	}
	t->initialized = false;
	if(!bin_file) return false;

	/* Clear the entire instance */
	memset(t, 0, sizeof(teenyat));

	/* backup .bin file */
	size_t words_read = fread(t->bin_image, sizeof(tny_word), TNY_RAM_SIZE, bin_file);
	if((words_read <= 0) || ferror(bin_file)) {
		return false;
	}

	/* default bus callbacks; replace with tny_bus_read()/tny_bus_write() */
	t->bus_read = default_bus_read;
	t->bus_write = default_bus_write;

	/*
	 * Initialize the clock_rate structure.  Note the epoch and
	 * last_calibration_time members are not set here as their times
	 * are set at the moment of the first clock cycle.  To keep this
	 * organized, lines that set them (below) are commented out.
	 */
	t->clock_rate.calibrate_cycles = TNY_DEFAULT_CALIBRATE_CYCLES;
	t->clock_rate.cycles_until_calibrate = TNY_DEFAULT_CALIBRATE_CYCLES;
	// t->clock_rate.epoch = <<< SET ON FIRST CLOCK CYCLE >>> ;
	// t->clock_rate.last_calibration_time = <<< SET ON FIRST CLOCK CYCLE >>> ;
	t->clock_rate.mhz_loop_cnt = tny_calibrate_1_us();

	if(!tny_reset(t)) {
		return false;
	}

	t->initialized = true;

	return true;
}

bool tny_init_unclocked(teenyat *t, FILE *bin_file) {

	if(!t) return false;

	bool result = tny_init_from_file(t,bin_file);
	t->control_status_register.csr.unclocked = 1;
	
	return result;
}

void tny_setup_random_generator(teenyat *t) {
	/*
	 * Initialize each teenyat with a unique random number stream
	 */
	static uint64_t stream = 0;

	/*
	 * find a random seed
	 */
	uint64_t seed = (intptr_t)&time << 32;
	seed ^= (intptr_t)&printf;
	seed ^= time(NULL);
	/* make seed "more" random w/ /dev/urandom if there */
	FILE *f = fopen("/dev/urandom", "rb");
	if(f != NULL) {
		uint64_t entropy_bits;
		if(fread(&entropy_bits, sizeof(entropy_bits), 1, f) == sizeof(entropy_bits)) {
			seed ^= entropy_bits;
		}
		fclose(f);
	}

	/* Set increment to arbitrary odd constant that goes up by stream */
	t->random.increment = ((intptr_t)&tny_reset + stream) | 1ULL;
	stream += 2;

	/*
	 * Set initial state and "put it in the past"
	 */
	t->random.state = seed + t->random.increment;
	(void)tny_random(t);

	return;
}

bool tny_reset(teenyat *t) {
	if(!t) return false;

	/* restore ram to it's initial post-bin-load state */
	memcpy(t->ram, t->bin_image, TNY_RAM_SIZE * sizeof(tny_word));

	t->reg[TNY_REG_PC].u = 0x0;
	t->reg[TNY_REG_SP].u = TNY_MAX_RAM_ADDRESS;
	t->reg[TNY_REG_ZERO].u = 0;
	t->reg[TNY_REG_A].u = 0;
	t->reg[TNY_REG_B].u = 0;
	t->reg[TNY_REG_C].u = 0;
	t->reg[TNY_REG_D].u = 0;
	t->reg[TNY_REG_E].u = 0;

	t->flags.carry   = false;
	t->flags.equals  = false;
	t->flags.less    = false;
	t->flags.greater = false;

	/* "instruction" and "immediate" members do not need initialization */

	/* Initialize ports to output w/ all 0s */
	t->port_a.u = 0;
	t->port_a_directions.u = 0;
	t->port_b.u = 0;
	t->port_b_directions.u = 0;

	t->port_change = NULL;

	/* Disable all architectural features */
	t->control_status_register.csr.interrupt_enable = 0;
	t->control_status_register.csr.interrupt_clearing = 0;
	t->control_status_register.csr.clock_divisor_scale = 0;
	t->control_status_register.csr.reserved = 0;

	/* Clear & disable all interrupts by default */
	t->interrupt_enable_register.u = 0;
	t->interrupt_queue_register.u  = 0;
	t->interrupt_return_address.u  = 0;
	t->interrupt_return_flags.carry   = false;
	t->interrupt_return_flags.equals  = false;
	t->interrupt_return_flags.less    = false;
	t->interrupt_return_flags.greater = false;
	/* Maybe dont memset? could simulate randomness... */
	memset(t->interrupt_vector_table, 0, sizeof(t->interrupt_vector_table));

	/* Initialize a unique random number generator stream */
	tny_setup_random_generator(t);

    /* Reset our timers */
    memset(t->timers, 0, sizeof(t->timers));

	/* Set up our initial calibrated cycles */
	t->clock_rate.cycles_until_calibrate = t->clock_rate.calibrate_cycles;

	t->delay_cycles = 0;
	t->cycle_cnt = 0;
	t->cycle_count_base = 0;

	return true;
}

void tny_modify_port_levels(teenyat *t, bool is_system_request, tny_word data, bool is_port_a) {
	tny_word *port = (is_port_a ? &(t->port_a) : &(t->port_b));
	tny_word dir = (is_port_a ? t->port_a_directions : t->port_b_directions);

	/* 
	 * For the masking below, we need a bit pattern of all 1s or 0s depending
	 * on whether this is a system request (1s) or not (0s).
	 */
	tny_word src;
	src.u = ~0 * is_system_request;

	/*
	 * We must know for which bits the source of the request and direction
	 * bits match.  Eg, if the system wants to modify the port levels,
	 * it can only do that for port bits with their direction set for input.
	 */
	tny_word src_dir_matches;
	src_dir_matches.u = src.u ^ dir.u;

	/* Save a copy of the port bits to determine whether any change */
	tny_word old_port = *port;

	/* Modify only those bits which should change */
	port->u = (port->u & src_dir_matches.u) + (data.u & ~src_dir_matches.u);

	/* Launch the port change callback if any port bits were modify */
	if((t->port_change != NULL) && (~src.u & ~dir.u & (old_port.u ^ port->u))) {
		t->port_change(t, is_port_a, *port);
	}

	return;
}

void tny_port_change(teenyat *t, TNY_PORT_CHANGE_FNPTR port_change) {
	t->port_change = port_change;

	return;
}

void tny_bus_read(teenyat *t, TNY_READ_FROM_BUS_FNPTR bus_read) {
	t->bus_read = bus_read ? bus_read : default_bus_read;

	return;
}

void tny_bus_write(teenyat *t, TNY_WRITE_TO_BUS_FNPTR bus_write) {
	t->bus_write = bus_write ? bus_write : default_bus_write;

	return;
}

void tny_get_ports(teenyat *t, tny_word *a, tny_word *b) {
	if(a != NULL) {
		*a = t->port_a;
	}

	if(b != NULL) {
		*b = t->port_b;
	}

	return;
}

void tny_set_ports(teenyat *t, tny_word *a, tny_word *b) {
	if(a != NULL) {
		tny_modify_port_levels(t, true, *a, true);
	}

	if(b != NULL) {
		tny_modify_port_levels(t, true, *b, false);
	}

	return;
}

void tny_set_timer_pin(teenyat *t, Timer timer, bool value) {
    t->timers[timer].t_in = value;
    return;
}

bool tny_get_timer_pin(teenyat *t, Timer timer) {
    return t->timers[timer].t_in;
}

void update_timer(teenyat* t, Timer timer, bool event_triggered) {
    tny_uword mode = t->timers[timer].csr.timer_csr.mode;
    if(mode == TIMER_MODE_COUNT || (mode == TIMER_MODE_LEVEL && t->timers[timer].t_in) || event_triggered) {
        t->timers[timer].count++;
        /* Trigger an interrupt if the timers value is reached */
        if(t->timers[timer].count >= t->timers[timer].compare) {
            tny_uword interrupt_mask = 1 << (timer + TNY_TIMER_A_INT);
			t->interrupt_queue_register.u |= interrupt_mask;
        }
    }
    return;
}

void handle_timers(teenyat* t) {
    for(int i = 0; i < TNY_TOTAL_TIMERS; i++) {
        bool reset = t->timers[i].csr.timer_csr.reset;
        bool enabled = t->timers[i].csr.timer_csr.enable;
        if(reset) {
            t->timers[i].csr.u   = 0;
            t->timers[i].compare = 0;
            t->timers[i].count   = 0;
        } else if(enabled) {
            tny_uword  cps   = t->timers[i].csr.timer_csr.clock_prescaler;
            /* treat a cps of 0 as 1 */
            if(cps == 0) {
                cps = 1;
            }
            /* update the timers internal clock */
            t->timers[i].internal_cycle_count++;
            if(t->timers[i].internal_cycle_count % cps == 0) {
                update_timer(t, i, false);
            }
        }
    }
    return;
}

void tny_trigger_timer_event(teenyat *t, Timer timer) {
    tny_uword mode = t->timers[timer].csr.timer_csr.mode;
    tny_uword enable = t->timers[timer].csr.timer_csr.enable;
    if(enable && mode == TIMER_MODE_EVENT) {
       update_timer(t, timer, true);
    }
    return;
}

void tny_external_interrupt(teenyat* t, tny_xint external_interrupt) {
	/*
	 * Make a mask with a 1 in the position of the external interrupt number
	 * offset so external interrupt 0..7 is mapped to bits 8..15 as the
	 * external eight interrupts are mapped to the interrupt vector table
	 * as ints 8..15.
	 * 
	 */
	tny_uword iqr_mask = 1U << ((external_interrupt % 8) + 8);
	/* mask in the interrupt into the upper half of our iqr */
	t->interrupt_queue_register.u |= iqr_mask;

	return;
}

/**
 * Priority scans a bit pattern and
 * returns the bit index of the least signifigant 1
 *
 * Assumes that bits is non-zero
 *
 * */
tny_uword tny_get_interrupt_index(tny_uword bits) {
	tny_uword n = 0;
	if (!(bits & 0x000000FF)) { n +=  8; bits >>=  8; }
	if (!(bits & 0x0000000F)) { n +=  4; bits >>=  4; }
	if (!(bits & 0x00000003)) { n +=  2; bits >>=  2; }
	if (!(bits & 0x00000001))   n +=  1;

	return n;
}

void handle_interrupts(teenyat *t) {
	bool      IE  = t->control_status_register.csr.interrupt_enable;
	bool      IC  = t->control_status_register.csr.interrupt_clearing;
	tny_uword IER = t->interrupt_enable_register.u;
	tny_uword IQR = t->interrupt_queue_register.u & IER;  //mask our queue register with enabled interrupts
	tny_uword INT = IQR & -IQR;  // get the highest priority interrupt
	if(IE && INT) {
		tny_uword ivt_index = tny_get_interrupt_index(INT);  // get the index into the ivt
		/* preserve our old program counter and flags */
		t->interrupt_return_address.u = t->reg[TNY_REG_PC].u;
		t->interrupt_return_flags = t->flags;
		/* jump to the corresponding ISR address */
		t->reg[TNY_REG_PC].u = t->interrupt_vector_table[ivt_index].u;
		t->control_status_register.csr.interrupt_enable = 0;  // disable interrupts
		t->interrupt_queue_register.u  &= ~INT;  // clear the request
	}

	/* clear interrupts if interrupt clearing is enabled */
	if(IC) {
		t->interrupt_queue_register.u &= IER;
	}

	return;
}

void tny_setup_clock_timing(teenyat *t) {
	uint64_t now = us_clock();
	t->clock_rate.epoch = now;
	t->wall_time_base = now;
	t->clock_rate.last_calibration_time = t->clock_rate.epoch;

	return;
}

void tny_adjust_clock_rate_stall(teenyat *t) {
	uint64_t now_us = us_clock();
	uint64_t us_elapsed = now_us - t->clock_rate.last_calibration_time;
	uint64_t mhz_loop_count = t->clock_rate.mhz_loop_cnt * t->clock_rate.calibrate_cycles;
	t->clock_rate.mhz_loop_cnt = mhz_loop_count / us_elapsed;

	uint64_t time_since_epoch_us = now_us - t->clock_rate.epoch;
	if(time_since_epoch_us > t->cycle_cnt) {
		/* too slow, speed up by busy looping 5% less */
		t->clock_rate.mhz_loop_cnt = (t->clock_rate.mhz_loop_cnt * 95) / 100;
	}
	else if(time_since_epoch_us < t->cycle_cnt) {
		/* too fast, slow down by busy looping 5% more*/
		/* NOTE: 1+ ensures even tiny busy_loop_count will at least go up by 1 */
		t->clock_rate.mhz_loop_cnt = 1 + (t->clock_rate.mhz_loop_cnt * 105) / 100;
	}

	t->clock_rate.last_calibration_time = now_us;
	t->clock_rate.cycles_until_calibrate = t->clock_rate.calibrate_cycles;

	return;
}

void tny_clock_rate_stall(teenyat *t) {
	/* Busy wait to fix the cycle rate */
	for(volatile uint64_t i = 0; i < t->clock_rate.mhz_loop_cnt; i++);

	return;
}

void tny_manage_clock_rate(teenyat *t) {
	if(--(t->clock_rate.cycles_until_calibrate) == 0) {
		tny_adjust_clock_rate_stall(t);
	}

	tny_clock_rate_stall(t);

	return;
}

void tny_clock(teenyat *t) {
	/* Setup clock timing on first cycle */
	if(t->cycle_cnt == 0){
		tny_setup_clock_timing(t);
	}

	t->cycle_cnt++;

    /* Timers are seperate pieces of internal hardware uneffected by delay cycles */
    handle_timers(t);

	/*
	 * If there were still cycles left on the previous instruction, skip
	 * everything else for now, and let those expire.
	 */
	if(t->delay_cycles) {
		t->delay_cycles--;
	}else{
		/*
		 * All instruction fetches are limited to the range 0x0000 through 0x7FFF.
		 * Modifications to the PC are always truncated to that range.  As such,
		 * an unusual circumstance could arrive where a two-word instruction begins
		 * at 0x7FFF and has its second word retrieved from 0x0000.  This almost
		 * certainly not something anyone would want, but it's how it works :-)
		 */

		handle_interrupts(t);

		trunc_pc(t);

		tny_uword orig_PC = t->reg[TNY_REG_PC].u;  // backup for error reporting

		tny_word IR = t->ram[t->reg[TNY_REG_PC].u];
		inc_pc(t);
		tny_sword immed = t->ram[t->reg[TNY_REG_PC].u].s;
		inc_pc(t);

		tny_uword opcode = IR.instruction.opcode;
		bool teeny = IR.instruction.teeny;
		tny_uword reg1 = IR.instruction.reg1;
		tny_uword reg2 = IR.instruction.reg2;
		bool carry = IR.inst_flags.carry;
		bool equals = IR.inst_flags.equals;
		bool less = IR.inst_flags.less;
		bool greater = IR.inst_flags.greater;

		if(teeny) {
			/*
			 * This is a single word instruction encoding
			 */
			dec_pc(t);
			immed = IR.instruction.immed4;
		}
		else {
			/* double word instructions cost one extra cycle */
			t->delay_cycles++;
		}

		/*
		 * EXECUTE
		 */

		uint32_t tmp;  /* for quick use to determine carry */

		switch(opcode) {
		case TNY_OPCODE_SET:
			t->reg[reg1].s = t->reg[reg2].s + immed;
			break;
		case TNY_OPCODE_LOD:
			{
				t->delay_cycles += TNY_BUS_DELAY;

				tny_uword addr = t->reg[reg2].s + immed;
				switch(addr) {
				case TNY_PORTA_ADDRESS:
					t->reg[reg1] = t->port_a;
					break;
				case TNY_PORTB_ADDRESS:
					t->reg[reg1] = t->port_b;
					break;
				case TNY_PORTA_DIR_ADDRESS:
					t->reg[reg1] = t->port_a_directions;
					break;
				case TNY_PORTB_DIR_ADDRESS:
					t->reg[reg1] = t->port_b_directions;
					break;
				case TNY_RANDOM_ADDRESS:
					t->reg[reg1].u = tny_random(t) & ((1 << 15) - 1);
					break;
				case TNY_RANDOM_BITS_ADDRESS:
					t->reg[reg1].u = tny_random(t);
					break;
				case TNY_CONTROL_STATUS_REGISTER:
					t->reg[reg1] = t->control_status_register;
					break;
				case TNY_INTERRUPT_ENABLE_REGISTER:
					t->reg[reg1] = t->interrupt_enable_register;
					break;
				case TNY_INTERRUPT_QUEUE_REGISTER:
					t->reg[reg1] = t->interrupt_queue_register;
					break;
				case TNY_CYCLE_COUNT:
				{
					uint64_t CD = 1ULL << (t->control_status_register.csr.clock_divisor_scale);
					/* Convert CPU cycles to timer cycles using the clock divisor */
					uint64_t cycles = (t->cycle_cnt - t->cycle_count_base) / CD;
					t->reg[reg1].u = (tny_uword)(cycles);
				}
					break; 	
				case TNY_WALL_TIME:
				{
					/* Convert microseconds to 1/16 second ticks */
					static const uint64_t us_per_tick = 1000000ULL / 16ULL; // 62500
					/* One tick is equal to 1/16 of a second */
					uint64_t ticks = (us_clock() - t->wall_time_base) / us_per_tick;
					t->reg[reg1].u = (tny_uword)(ticks);
					break;
				}
                case TNY_TIMER_A_CSR:
					t->reg[reg1] = t->timers[TNY_TIMER_A].csr;
                    break;
                case TNY_TIMER_A_CMP:
					t->reg[reg1].u = t->timers[TNY_TIMER_A].compare;
                    break;
                case TNY_TIMER_A_CNT:
					t->reg[reg1].u = t->timers[TNY_TIMER_A].count;
                    break;
                case TNY_TIMER_B_CSR:
					t->reg[reg1] = t->timers[TNY_TIMER_B].csr;
                    break;
                case TNY_TIMER_B_CMP:
					t->reg[reg1].u = t->timers[TNY_TIMER_B].compare;
                    break;
                case TNY_TIMER_B_CNT:
					t->reg[reg1].u = t->timers[TNY_TIMER_B].count;
                    break;
				default:
					/* Check if reading from interrupt service */
					if(addr >= TNY_INTERRUPT_VECTOR_TABLE_START &&
					   addr <= TNY_INTERRUPT_VECTOR_TABLE_END
					  ) {
						t->reg[reg1] = t->interrupt_vector_table[addr - TNY_INTERRUPT_VECTOR_TABLE_START];
					}
					else if(addr >= TNY_PERIPHERAL_BASE_ADDRESS) {
						/* read from peripheral address */
						t->delay_cycles += TNY_BUS_EXTERNAL_DELAY_ADJUST;

						tny_word data = {.u = 0};
						uint16_t delay = 0;
						t->bus_read(t, addr, &data, &delay);
						t->reg[reg1] = data;
						t->delay_cycles += delay;
					}
					else if(addr <= TNY_MAX_RAM_ADDRESS) {
						/* read from RAM */
						t->delay_cycles += TNY_BUS_EXTERNAL_DELAY_ADJUST;

						t->reg[reg1] = t->ram[addr];
					}
					else {
						/* 
						 * This is an attempt to access an unaccounted for
						 * address in the "Microcontroller Device Space".
						 */
					}
					break;
				}
			}
			break;
		case TNY_OPCODE_STR:
			{
				/*
				 * To promote student use of registers, all bus operations,
				 * including RAM access comes with an extra penalty.
				 */
				t->delay_cycles += TNY_BUS_DELAY;

				tny_uword addr = t->reg[reg1].s + immed;
				switch(addr) {
				case TNY_PORTA_ADDRESS:
					tny_modify_port_levels(t, false, t->reg[reg2], true);
					break;
				case TNY_PORTB_ADDRESS:
					tny_modify_port_levels(t, false, t->reg[reg2], false);
					break;
				case TNY_PORTA_DIR_ADDRESS:
					t->port_a_directions = t->reg[reg2];
					break;
				case TNY_PORTB_DIR_ADDRESS:
					t->port_b_directions = t->reg[reg2];
					break;
				case TNY_RANDOM_ADDRESS:
					/* Do nothing */
					break;
				case TNY_RANDOM_BITS_ADDRESS:
					/* Do nothing */
					break;
				case TNY_CONTROL_STATUS_REGISTER:
				{
					bool u1 = t->control_status_register.csr.unclocked;
					t->control_status_register = t->reg[reg2];

					/*
					 * Reset CPU rate timing control if changing from unclocked
					 * to clocked mode.
					 */
					bool u2 = t->control_status_register.csr.unclocked;
					if(u1 && !u2) {
						t->clock_rate.cycles_until_calibrate = t->clock_rate.calibrate_cycles;
						uint64_t now = us_clock();
						t->clock_rate.epoch = now;
						t->clock_rate.last_calibration_time = now;
					}
					break;
				}
				case TNY_INTERRUPT_ENABLE_REGISTER:
					t->interrupt_enable_register = t->reg[reg2];
					break;
				case TNY_INTERRUPT_QUEUE_REGISTER:
					t->interrupt_queue_register = t->reg[reg2];
					break;
				case TNY_CYCLE_COUNT_RESET:
					t->cycle_count_base = t->cycle_cnt;
					break;
				case TNY_WALL_TIME_RESET:
					t->wall_time_base = us_clock();
					break;
                case TNY_TIMER_A_CSR:
					t->timers[TNY_TIMER_A].csr = t->reg[reg2];
                    break;
                case TNY_TIMER_A_CMP:
					t->timers[TNY_TIMER_A].compare = t->reg[reg2].u;
                    break;
                case TNY_TIMER_A_CNT:
					t->timers[TNY_TIMER_A].count  = t->reg[reg2].u;
                    break;
                case TNY_TIMER_B_CSR:
					t->timers[TNY_TIMER_B].csr = t->reg[reg2];
                    break;
                case TNY_TIMER_B_CMP:
					t->timers[TNY_TIMER_B].compare = t->reg[reg2].u;
                    break;
                case TNY_TIMER_B_CNT:
				    t->timers[TNY_TIMER_B].count = t->reg[reg2].u;
                    break;
				default:
					/* Check if writing to interrupt service */
					if(addr >= TNY_INTERRUPT_VECTOR_TABLE_START &&
					   addr <= TNY_INTERRUPT_VECTOR_TABLE_END
					  ) {
						t->interrupt_vector_table[addr - TNY_INTERRUPT_VECTOR_TABLE_START] = t->reg[reg2];
					}
					else if(addr >= TNY_PERIPHERAL_BASE_ADDRESS) {
						/* write to peripheral address */
						t->delay_cycles += TNY_BUS_EXTERNAL_DELAY_ADJUST;

						uint16_t delay = 0;
						t->bus_write(t, addr, t->reg[reg2], &delay);
						t->delay_cycles += delay;
					}
					else if(addr <= TNY_MAX_RAM_ADDRESS) {
						/* write to RAM */
						t->delay_cycles += TNY_BUS_EXTERNAL_DELAY_ADJUST;

						t->ram[addr] = t->reg[reg2];
					}
					else {
						/* 
						 * This is an attempt to access an unaccounted for
						 * address in the "Microcontroller Device Space".
						 */
					}
					break;
				}
			}
			break;
		case TNY_OPCODE_PSH:
			{
				tny_word tmp;
				tmp.s = t->reg[reg2].s + immed;
				t->reg[reg1].u &= TNY_MAX_RAM_ADDRESS;
				t->ram[t->reg[reg1].u] = tmp;
				t->reg[reg1].u--;
				t->reg[reg1].u &= TNY_MAX_RAM_ADDRESS;
				/*
				* To promote student use of registers, all bus operations,
				* including RAM access comes with an extra penalty.
				*/
				t->delay_cycles += TNY_BUS_DELAY;
				break;
			}
		case TNY_OPCODE_POP:
			t->reg[reg2].u++;
			t->reg[reg2].u &= TNY_MAX_RAM_ADDRESS;
			t->reg[reg1] = t->ram[t->reg[reg2].u];  // POP ignores immediate
			/*
			 * To promote student use of registers, all bus operations,
			 * including RAM access comes with an extra penalty.
			 */
			t->delay_cycles += TNY_BUS_DELAY;
			break;
		case TNY_OPCODE_BTS:
			{
				tny_sword bit = t->reg[reg2].s + immed;
				if(bit >= 0 && bit <= 15) {
					t->reg[reg1].s |= (1 << bit);
					set_elg_flags(t, t->reg[reg1].s);
				}
			}
			break;
		case TNY_OPCODE_BTC:
			{
				tny_sword bit = t->reg[reg2].s + immed;
				if(bit >= 0 && bit <= 15) {
					t->reg[reg1].s &= ~(1 << bit);
					set_elg_flags(t, t->reg[reg1].s);
				}
			}
			break;
		case TNY_OPCODE_BTF:
			{
				tny_sword bit = t->reg[reg2].s + immed;
				if(bit >= 0 && bit <= 15) {
					t->reg[reg1].s ^= (1 << bit);
					set_elg_flags(t, t->reg[reg1].s);
				}
			}
			break;
		case TNY_OPCODE_CAL:
			t->reg[TNY_REG_SP].u &= TNY_MAX_RAM_ADDRESS;
			t->ram[t->reg[TNY_REG_SP].u] = t->reg[TNY_REG_PC];
			t->reg[TNY_REG_SP].u--;
			t->reg[TNY_REG_SP].u &= TNY_MAX_RAM_ADDRESS;
			set_pc(t, t->reg[reg2].s + immed);
			/*
			 * To promote student use of registers, all bus operations,
			 * including RAM access comes with an extra penalty.
			 */
			t->delay_cycles += TNY_BUS_DELAY;
			break;
		case TNY_OPCODE_ADD:
			{
				uint16_t right = t->reg[reg2].s + immed;
				uint16_t left = t->reg[reg1].u;
				tmp = (uint32_t)left + (uint32_t)right;
				t->flags.carry = tmp & (1 << 16);
				t->reg[reg1].s = tmp;
				set_elg_flags(t, t->reg[reg1].s);
				break;
			}
		case TNY_OPCODE_SUB:
			{
				uint16_t right = t->reg[reg2].s + immed;
				right = -right;
				uint16_t left = t->reg[reg1].u;
				tmp = (uint32_t)left + (uint32_t)right;
				t->flags.carry = tmp & (1 << 16);
				t->reg[reg1].s = tmp;
				set_elg_flags(t, t->reg[reg1].s);
				break;
			}
		case TNY_OPCODE_MPY:
			{
				/* The carry flag is undefined for TeenyAT multiplication */
				int16_t right = t->reg[reg2].s + immed;
				int16_t left = t->reg[reg1].s;
				t->reg[reg1].u = left * right;
				set_elg_flags(t, t->reg[reg1].s);
				break;
			}
		case TNY_OPCODE_DIV:
			/* The carry flag is undefined for TeenyAT division */
			if(t->reg[reg2].s + immed != 0) {
				t->reg[reg1].s /= t->reg[reg2].s + immed;
				set_elg_flags(t, t->reg[reg1].s);
			}
			else {
				/* No behavior defined on divide-by-zero */
			}
			break;
		case TNY_OPCODE_MOD:
			/* The carry flag is undefined for TeenyAT division */
			if(t->reg[reg2].s + immed != 0) {
				t->reg[reg1].s %= t->reg[reg2].s + immed;
				set_elg_flags(t, t->reg[reg1].s);
			}
			else {
				/* No behavior defined on divide-by-zero */
			}
			break;
		case TNY_OPCODE_AND:
			t->reg[reg1].s &= t->reg[reg2].s + immed;
			set_elg_flags(t, t->reg[reg1].s);
			break;
		case TNY_OPCODE_OR:
			t->reg[reg1].s |= t->reg[reg2].s + immed;
			set_elg_flags(t, t->reg[reg1].s);
			break;
		case TNY_OPCODE_XOR:
			t->reg[reg1].s ^= t->reg[reg2].s + immed;
			set_elg_flags(t, t->reg[reg1].s);
			break;
		case TNY_OPCODE_SHF:
			{
				tny_sword bits_to_shift = t->reg[reg2].s + immed;
				if(bits_to_shift < 0) {
					/* shift left */
					bits_to_shift *= -1;
					if(bits_to_shift <= 15) {
						t->reg[reg1].u <<= bits_to_shift - 1;
						t->flags.carry = (t->reg[reg1].u >> 15) & 1;
						t->reg[reg1].u <<= 1;
					}
					else {
						if(bits_to_shift == 16) {
							t->flags.carry = t->reg[reg1].u & (1 << 0);
						}
						else {
							t->flags.carry = 0;
						}
						t->reg[reg1].u = 0;
					}
				}
				else if(bits_to_shift > 0) {
					/* shift right */
					if(bits_to_shift <= 15) {
						t->reg[reg1].u >>= bits_to_shift - 1;
						t->flags.carry = t->reg[reg1].u & (1 << 0);
						t->reg[reg1].u >>= 1;
					}
					else {
						if(bits_to_shift == 16) {
							t->flags.carry = (t->reg[reg1].u >> 15) & 1;
						}
						else {
							t->flags.carry = 0;
						}
						t->reg[reg1].u = 0;
					}
				}
				set_elg_flags(t, t->reg[reg1].s);
			}
			break;
		case TNY_OPCODE_ROT:
			{
				/* calculate remainder as rotate could go around many times */
				tny_sword bits_to_rotate = (t->reg[reg2].s + immed) % 16;
				if(bits_to_rotate < 0) {
					/* rotate left */
					bits_to_rotate *= -1;
					tny_uword main_part = t->reg[reg1].u << bits_to_rotate;
					tny_uword wrap_part = t->reg[reg1].u >> (16 - bits_to_rotate);
					t->reg[reg1].u = main_part | wrap_part;
					t->flags.carry = t->reg[reg1].u & (1 << 0);
				}
				else if(bits_to_rotate > 0) {
					/* rotate right */
					tny_uword main_part = t->reg[reg1].u >> bits_to_rotate;
					tny_uword wrap_part = t->reg[reg1].u << (16 - bits_to_rotate);
					t->reg[reg1].u = main_part | wrap_part;
					t->flags.carry = (t->reg[reg1].u >> 15) & 1;
				}
				set_elg_flags(t, t->reg[reg1].s);
			}
			break;
		case TNY_OPCODE_NEG:
			/* The carry flag is undefined for TeenyAT negation */
			tmp = (uint32_t)0 - (uint32_t)(t->reg[reg1].s);
			t->reg[reg1].s = tmp;
			set_elg_flags(t, t->reg[reg1].s);
			break;
		case TNY_OPCODE_CMP:
			{
				uint16_t right = t->reg[reg2].s + immed;
				right = -right;
				uint16_t left = t->reg[reg1].u;
				tmp = (uint32_t)left + (uint32_t)right; 
				t->flags.carry = tmp & (1 << 16);
				set_elg_flags(t, (tny_sword)tmp);
				break;
			}
		case TNY_OPCODE_JMP:
			{
				bool flags_checked = false;
				bool condition_satisfied = false;
				if(carry) {
					flags_checked = true;
					condition_satisfied |= t->flags.carry;
				}
				if(equals) {
					flags_checked = true;
					condition_satisfied |= t->flags.equals;
				}
				if(less) {
					flags_checked = true;
					condition_satisfied |= t->flags.less;
				}
				if(greater) {
					flags_checked = true;
					condition_satisfied |= t->flags.greater;
				}
				if(!flags_checked || condition_satisfied) {
					set_pc(t, t->reg[reg1].s + immed);
				}
			}
			break;
		case TNY_OPCODE_LUP:
			/* The carry flag is undefined for TeenyAT LUP-ing */
			t->reg[reg1].s = (uint32_t)(t->reg[reg1].s) - 1;
			set_elg_flags(t, t->reg[reg1].s);
			if(t->reg[reg1].s != 0) {
				set_pc(t, t->reg[reg2].s + immed);
			}
			break;
		case TNY_OPCODE_DLY:
			{
				tny_uword delay_prescale = t->reg[reg1].u;
				if(delay_prescale == 0) {
					delay_prescale = 1;
				}
				tny_uword delay_cnt = (tny_uword)(t->reg[reg2].s + immed);
				uint64_t prescaled_delay_cycles = delay_prescale * delay_cnt;
				if(prescaled_delay_cycles >= 1) {
					/* current instruction already 1 cycle */
					t->delay_cycles += prescaled_delay_cycles - 1;
				}
			}
			break;
		case TNY_OPCODE_INT:
			{
				tny_sword interrupt_number = t->reg[reg2].s + immed;

				/*
				 * Make a mask with a 1 in the position of the interrupt number
				 *
				 */

                if(interrupt_number >= 0 && interrupt_number < 16) {
                    tny_uword interrupt_mask = 1U << interrupt_number;
                    /* mask in the interrupt into the upper half of our iqr */
                    t->interrupt_queue_register.u |= interrupt_mask;
                }
			}
			break;
		case TNY_OPCODE_RTI:
			set_pc(t, t->interrupt_return_address.u);  // restore pc
			t->flags = t->interrupt_return_flags;     // restore flags
			t->control_status_register.csr.interrupt_enable = 1;  // reenable interrupts
			break;
		default:
			fprintf(stderr, "Unknown opcode (%d) encountered at 0x%04X on cycle %" PRIu64 "\n",
					opcode, orig_PC, t->cycle_cnt);
			break;
		}

		/* Ensure the zero register still has a zero in it */
		t->reg[TNY_REG_ZERO].u = 0;
	}

	/* Jump out if unclocked instance of the TeenyAT */
	if(!t->control_status_register.csr.unclocked) {
		tny_manage_clock_rate(t);
	}
	
	return;
}

tny_uword tny_random(teenyat *t) {
	uint64_t tmp = t->random.state;

	/*
	 * Find the next state in the sequence.  The weird large immediate value
	 * is a special constant chosen by the world at large to do great
	 * things for the random number. ... We don't really know about it,
	 * but it seems to work ;-)
	 */
	t->random.state = tmp * 6364136223846793005ULL + t->random.increment;

	/*
	 * The code below involves some shifts of seemingly random amounts.
	 * They determine the amounts of manipulation of 64 and 16 bit values.
	 * 27 = 32 - 5
	 * 18 = floor((64 - 27) / 2)
	 * 59 = 64 - 5
	 */

	/* use top 5 bits of previous state to "randomly" rotate */
	unsigned bitcnt_to_rotate = tmp >> 59;

	/* scramble the previous state and truncate to 16 bits */
	uint32_t to_rotate = (tmp >> 18 ^ tmp) >> 27;

	/* return the right-rotated the scrambled previous state */
	uint32_t result = to_rotate >> bitcnt_to_rotate;
	/* the bitmask below ensures the left shift is fewer than 32 bits */
	result |= to_rotate << (-bitcnt_to_rotate & ~(~0U << 5));

	/* truncate result and return as 16-bit tny_uword */
	return (tny_uword)result;
}

/*
 * This function will estimate the number of iterations needed in
 * a busy loop to consume 1 us (clock period for 1 MHz).
 */ 
uint64_t tny_calibrate_1_us(void){
	const uint64_t TRIAL_CNT = 5212004;
	uint64_t start = us_clock();

	/* consume some real world time with empty loop */
	for(volatile uint64_t i = 0; i < TRIAL_CNT; i++) ;

	return TRIAL_CNT / (us_clock() - start + 1);  // +1 to avoid 0 denominator
}
