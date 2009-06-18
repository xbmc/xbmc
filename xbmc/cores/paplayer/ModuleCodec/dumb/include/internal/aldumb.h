/*  _______         ____    __         ___    ___
 * \    _  \       \    /  \  /       \   \  /   /       '   '  '
 *  |  | \  \       |  |    ||         |   \/   |         .      .
 *  |  |  |  |      |  |    ||         ||\  /|  |
 *  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
 *  |  |  |  |      |  |    ||         ||    |  |         .      .
 *  |  |_/  /        \  \__//          ||    |  |
 * /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
 *                                                      /  \
 *                                                     / .  \
 * internal/aldumb.h - The internal header file       / / \  \
 *                     for DUMB with Allegro.        | <  /   \_
 *                                                   |  \/ /\   /
 * This header file provides access to the            \_  /  > /
 * internal structure of DUMB, and is liable            | \ / /
 * to change, mutate or cease to exist at any           |  ' /
 * moment. Include it at your own peril.                 \__/
 *
 * ...
 *
 * Seriously. You don't need access to anything in this file. All right, you
 * probably do actually. But if you use it, you will be relying on a specific
 * version of DUMB, so please check DUMB_VERSION defined in dumb.h. Please
 * contact the authors so that we can provide a public API for what you need.
 */

#ifndef INTERNAL_ALDUMB_H
#define INTERNAL_ALDUMB_H


void _dat_unload_duh(void *duh);


#endif /* INTERNAL_DUMB_H */
