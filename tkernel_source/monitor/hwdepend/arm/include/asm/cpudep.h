/*
 *----------------------------------------------------------------------
 *    T-Kernel 2.0 Software Package
 *
 *    Copyright 2011 by Ken Sakamura.
 *    This software is distributed under the latest version of T-License 2.x.
 *----------------------------------------------------------------------
 *
 *    Released by T-Engine Forum(http://www.t-engine.org/) at 2011/05/17.
 *    Modified by TRON Forum(http://www.tron.org/) at 2015/06/01.
 *
 *----------------------------------------------------------------------
 */

/*
 *	cpudep.h
 *
 *       CPU-dependent definitions(ARM)
 */

#include <tk/sysdef.h>

#ifndef	_in_asm_source_
#include "cpudepend.h"

IMPORT W	bootFlag;	/* boot flag */

/*
 * Memory access through physical address
 *       In the case of ARM, actually it is an access by logical address.
 */

#define rd_w(pa) in_w(pa)
#define rd_h(pa) in_h(pa)
#define rd_b(pa) in_b(pa)

#define wr_w(pa, data) out_w(pa, data)
#define wr_h(pa, data) out_h(pa, data)
#define wr_b(pa, data) out_b(pa, data)

/*
 * read/write the ARM-specific registered under monitor management
 *      read/set the value of registers at the time of monitor entry.
 */
IMPORT UW   getCP15( W reg, W opcd );	/* CP15 register reg: CRn, opcd: Op2 */
IMPORT UW   getCurPCX( void );		/* PC register (unmodified) */
IMPORT void setCurPCX( UW val );	/* PC register (unmodified) */
IMPORT UW   getCurCPSR( void );		/* CPSR register */
IMPORT UW   getCurSPSR( void );		/* SPSR register */

/*
 * Validate PC address
 *       Allow only ARM instructions (on 4 bytes boundary).
 *      If addr is valid then return 0, otherwise return -1.
 */
IMPORT W invalidPC2( UW addr );

/*
 * obtain step address
 */
IMPORT W getStepAddr( UW pc, UW cpsr, W mode, UW* npc, UW *rep );

#endif	/* _in_asm_source_ */
