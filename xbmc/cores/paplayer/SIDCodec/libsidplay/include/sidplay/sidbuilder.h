/***************************************************************************
                          sidbuilder.h  -  Sid Builder Classes
                             -------------------
    begin                : Sat May 6 2001
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

#ifndef _sidbuilder_h_
#define _sidbuilder_h_

#include "sid2types.h"
#include "component.h"
#include "c64env.h"


// Inherit this class to create a new SID emulations for libsidplay2.
class sidbuilder;
class sidemu: public component
{
private:
    sidbuilder *m_builder;

public:
    sidemu (sidbuilder *builder)
    :m_builder (builder) {;}
    virtual ~sidemu () {;}

    // Standard component functions
    void            reset () { reset (0); }
    virtual void    reset (uint8_t volume) = 0;
    virtual uint8_t read  (uint_least8_t addr) = 0;
    virtual void    write (uint_least8_t addr, uint8_t data) = 0;
    virtual const   char *credits (void) = 0;

    // Standard SID functions
    virtual int_least32_t output  (uint_least8_t bits) = 0;
    virtual void          voice   (uint_least8_t num,
                                   uint_least8_t vol,
                                   bool mute) = 0;
    virtual void          gain    (int_least8_t precent) = 0;
    virtual void          optimisation (uint_least8_t /*level*/) {;}
    sidbuilder           *builder (void) const { return m_builder; }
};


class sidbuilder
{
private:
    const char * const m_name;

protected:
    bool m_status;

public:
    // Determine current state of object (true = okay, false = error).
    sidbuilder(const char * const name)
        : m_name(name), m_status (true) {;}
    virtual ~sidbuilder() {;}

    virtual  sidemu      *lock    (c64env *env, sid2_model_t model) = 0;
    virtual  void         unlock  (sidemu *device) = 0;
    const    char        *name    (void) const { return m_name; }
    virtual  const  char *error   (void) const = 0;
    virtual  const  char *credits (void) = 0;
    operator bool() const { return m_status; }
};

#endif // _sidbuilder_h_
