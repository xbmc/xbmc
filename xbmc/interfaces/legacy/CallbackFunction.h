/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"

namespace XBMCAddon
{
  /**
   * <p>This is the parent class for the class templates that hold
   *   a callback. A callback is essentially a templatized
   *   functor (functoid?) for a call to a member function.</p>
   *
   * <p>This class combined with the attending CallbackHandlers should make
   *  sure that the AddonClass isn't in the midst of deallocating when
   *  the callback executes. In this way the Callback class acts as
   *  a weak reference.</p>
   */
  class Callback : public AddonClass
  {
  protected:
    AddonClass* addonClassObject;
    explicit Callback(AddonClass* _object) : addonClassObject(_object) { XBMC_TRACE; }

  public:
    virtual void executeCallback() = 0;
    ~Callback() override;

    AddonClass* getObject() { XBMC_TRACE; return addonClassObject; }
  };

  struct cb_null_type {};

  // stub type template to be partial specialized
  template<typename M = cb_null_type, typename T1 = cb_null_type,
           typename T2 = cb_null_type, typename T3 = cb_null_type,
           typename T4 = cb_null_type, typename Extraneous = cb_null_type>
  class CallbackFunction {};

  /**
   * This is the template to carry a callback to a member function
   *  that returns 'void' (has no return) and takes no parameters.
   */
  template<class M> class CallbackFunction<M, cb_null_type, cb_null_type, cb_null_type, cb_null_type, cb_null_type> : public Callback
  {
  public:
    typedef void (M::*MemberFunction)();

  protected:
    MemberFunction meth;
    M* obj;

  public:
    CallbackFunction(M* object, MemberFunction method) :
      Callback(object), meth(method), obj(object) { XBMC_TRACE; }

    ~CallbackFunction() override { XBMC_TRACE; deallocating(); }

    void executeCallback() override { XBMC_TRACE; ((*obj).*(meth))(); }
  };

  /**
   * This is the template to carry a callback to a member function
   *  that returns 'void' (has no return) and takes one parameter.
   */
  template<class M, typename P1> class CallbackFunction<M,P1, cb_null_type, cb_null_type, cb_null_type, cb_null_type> : public Callback
  {
  public:
    typedef void (M::*MemberFunction)(P1);

  protected:
    MemberFunction meth;
    M* obj;
    P1 param;

  public:
    CallbackFunction(M* object, MemberFunction method, P1 parameter) :
      Callback(object), meth(method), obj(object),
      param(parameter) { XBMC_TRACE; }

    ~CallbackFunction() override { XBMC_TRACE; deallocating(); }

    void executeCallback() override { XBMC_TRACE; ((*obj).*(meth))(param); }
  };

  /**
   * This is the template to carry a callback to a member function
   *  that returns 'void' (has no return) and takes one parameter
   *  that can be held in an AddonClass::Ref
   */
  template<class M, typename P1> class CallbackFunction<M,AddonClass::Ref<P1>, cb_null_type, cb_null_type, cb_null_type, cb_null_type> : public Callback
  {
  public:
    typedef void (M::*MemberFunction)(P1*);

  protected:
    MemberFunction meth;
    M* obj;
    AddonClass::Ref<P1> param;

  public:
    CallbackFunction(M* object, MemberFunction method, P1* parameter) :
      Callback(object), meth(method), obj(object),
      param(parameter) { XBMC_TRACE; }

    ~CallbackFunction() override { XBMC_TRACE; deallocating(); }

    void executeCallback() override { XBMC_TRACE; ((*obj).*(meth))(param); }
  };


  /**
   * This is the template to carry a callback to a member function
   *  that returns 'void' (has no return) and takes two parameters.
   */
  template<class M, typename P1, typename P2> class CallbackFunction<M,P1,P2, cb_null_type, cb_null_type, cb_null_type> : public Callback
  {
  public:
    typedef void (M::*MemberFunction)(P1,P2);

  protected:
    MemberFunction meth;
    M* obj;
    P1 param1;
    P2 param2;

  public:
    CallbackFunction(M* object, MemberFunction method, P1 parameter, P2 parameter2) :
      Callback(object), meth(method), obj(object),
      param1(parameter), param2(parameter2) { XBMC_TRACE; }

    ~CallbackFunction() override { XBMC_TRACE; deallocating(); }

    void executeCallback() override { XBMC_TRACE; ((*obj).*(meth))(param1,param2); }
  };


  /**
   * This is the template to carry a callback to a member function
   *  that returns 'void' (has no return) and takes three parameters.
   */
  template<class M, typename P1, typename P2, typename P3> class CallbackFunction<M,P1,P2,P3, cb_null_type, cb_null_type> : public Callback
  {
  public:
    typedef void (M::*MemberFunction)(P1,P2,P3);

  protected:
    MemberFunction meth;
    M* obj;
    P1 param1;
    P2 param2;
    P3 param3;

  public:
    CallbackFunction(M* object, MemberFunction method, P1 parameter, P2 parameter2, P3 parameter3) :
      Callback(object), meth(method), obj(object),
      param1(parameter), param2(parameter2), param3(parameter3) { XBMC_TRACE; }

    ~CallbackFunction() override { XBMC_TRACE; deallocating(); }

    void executeCallback() override { XBMC_TRACE; ((*obj).*(meth))(param1,param2,param3); }
  };
}


