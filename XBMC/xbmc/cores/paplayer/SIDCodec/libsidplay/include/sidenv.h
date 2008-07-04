/***************************************************************************
                          environment.h - This is the environment file which
                                          defines all the standard functions
                                          to be inherited by the ICs.
                             -------------------
    begin                : Thu May 11 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 *  $Log: sidenv.h,v $
 *  Revision 1.5  2002/01/29 21:53:25  s_a_white
 *  Fixed envSleep
 *
 *  Revision 1.4  2002/01/29 08:02:22  s_a_white
 *  PSID sample improvements.
 *
 *  Revision 1.3  2001/07/14 13:09:35  s_a_white
 *  Removed cache parameters.
 *
 *  Revision 1.2  2000/12/11 19:10:59  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _environment_h_
#define _environment_h_

#include "sidtypes.h"

/*
// Enviroment functions - THESE FUNTIONS MUST BE PROVIDED
// TO ALLOW THE COMPONENTS TO SPEAK TO EACH OTHER.  ENVP
// CAN BE USED TO CREATE VERSIONS OF THESE FUNTIONS
// WHICH ACCESS MEMBER FUNTIONS OF OTHER C++ OBJECTS!
extern void    reset        (void);
extern uint8_t readMemByte  (uint_least16_t addr);
extern void    writeMemByte (uint_least16_t addr, uint8_t data);

// Interrupts - You must raise the interrupt(s)
// every cycle if you have not yet been serviced
extern void  triggerIRQ (void);
extern void  triggerNMI (void);
extern void  triggerRST (void);
extern void  clearIRQ   (void);

// Sidplay compatibily funtions
extern bool    checkBankJump  (uint_least16_t addr);
extern uint8_t readEnvMemByte (uint_least16_t addr);
*/

class C64Environment
{
/*
protected:
    // Eniviroment functions
    virtual inline void    envReset        (void)
    { ::reset (); }
    virtual inline uint8_t envReadMemByte  (uint_least16_t addr)
    { ::readMemByte  (addr); }
    virtual inline void    envWriteMemByte (uint_least16_t addr, uint8_t data)
    { ::writeMemByte (addr, data); }

    // Interrupts
    virtual inline void  encTriggerIRQ (void)
    { ::triggerIRQ (); }
    virtual inline void  envTriggerNMI (void)
    { ::triggerNMI (); }
    virtual inline void  envTriggerRST (void)
    { ::triggerRST (); }
    virtual inline void  envClearIRQ   (void)
    { ::clearIRQ   (); }

    // Sidplay compatibily funtions
    virtual inline bool    envCheckBankJump   (uint_least16_t addr)
    { ::checkBankJump   (); }
    virtual inline uint8_t envReadMemDataByte (uint_least16_t addr)
    { ::readMemDataByte (); }
    */

private:
    C64Environment *m_envp;

    // Sidplay2 Player Environemnt
public:
    virtual ~C64Environment () {}
    virtual void setEnvironment (C64Environment *envp)
    {
        m_envp = envp;
    }

protected:
    // Eniviroment functions
    virtual inline void  envReset   (void)
    { m_envp->envReset (); }
    virtual inline uint8_t envReadMemByte  (uint_least16_t addr)
    { return m_envp->envReadMemByte (addr); }
    virtual inline void    envWriteMemByte (uint_least16_t addr, uint8_t data)
    { m_envp->envWriteMemByte (addr, data); }

    // Interrupts
    virtual inline void  envTriggerIRQ (void)
    { m_envp->envTriggerIRQ (); }
    virtual inline void  envTriggerNMI (void)
    { m_envp->envTriggerNMI (); }
    virtual inline void  envTriggerRST (void)
    { m_envp->envTriggerRST (); }
    virtual inline void  envClearIRQ   (void)
    { m_envp->envClearIRQ ();   }

    // Sidplay compatibily funtions
    virtual inline bool    envCheckBankJump   (uint_least16_t addr)
    { return m_envp->envCheckBankJump   (addr); }
    virtual inline uint8_t envReadMemDataByte (uint_least16_t addr)
    { return m_envp->envReadMemDataByte (addr); }
    virtual inline void envSleep (void)
    { m_envp->envSleep (); }
    virtual inline void envLoadFile (char *file)
    { m_envp->envLoadFile (file); }
};

#endif // _environment_h_
