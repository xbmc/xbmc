/* algebra.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* based on Wei Dai's algebra.h from CryptoPP */

#ifndef TAO_CRYPT_ALGEBRA_HPP
#define TAO_CRYPT_ALGEBRA_HPP

#include "integer.hpp"

namespace TaoCrypt {


// "const Element&" returned by member functions are references
// to internal data members. Since each object may have only
// one such data member for holding results, the following code
// will produce incorrect results:
// abcd = group.Add(group.Add(a,b), group.Add(c,d));
// But this should be fine:
// abcd = group.Add(a, group.Add(b, group.Add(c,d));

// Abstract Group
class TAOCRYPT_NO_VTABLE AbstractGroup : public virtual_base
{
public:
    typedef Integer Element;

    virtual ~AbstractGroup() {}

    virtual bool Equal(const Element &a, const Element &b) const =0;
    virtual const Element& Identity() const =0;
    virtual const Element& Add(const Element &a, const Element &b) const =0;
    virtual const Element& Inverse(const Element &a) const =0;
    virtual bool InversionIsFast() const {return false;}

    virtual const Element& Double(const Element &a) const;
    virtual const Element& Subtract(const Element &a, const Element &b) const;
    virtual Element& Accumulate(Element &a, const Element &b) const;
    virtual Element& Reduce(Element &a, const Element &b) const;

    virtual Element ScalarMultiply(const Element &a, const Integer &e) const;
    virtual Element CascadeScalarMultiply(const Element &x, const Integer &e1,
                                    const Element &y, const Integer &e2) const;

    virtual void SimultaneousMultiply(Element *results, const Element &base,
                  const Integer *exponents, unsigned int exponentsCount) const;
};

// Abstract Ring
class TAOCRYPT_NO_VTABLE AbstractRing : public AbstractGroup
{
public:
    typedef Integer Element;

    AbstractRing() : AbstractGroup() {m_mg.m_pRing = this;}
    AbstractRing(const AbstractRing &source) : AbstractGroup()
                                                {m_mg.m_pRing = this;}
    AbstractRing& operator=(const AbstractRing &source) {return *this;}

    virtual bool IsUnit(const Element &a) const =0;
    virtual const Element& MultiplicativeIdentity() const =0;
    virtual const Element& Multiply(const Element&, const Element&) const =0;
    virtual const Element& MultiplicativeInverse(const Element &a) const =0;

    virtual const Element& Square(const Element &a) const;
    virtual const Element& Divide(const Element &a, const Element &b) const;

    virtual Element Exponentiate(const Element &a, const Integer &e) const;
    virtual Element CascadeExponentiate(const Element &x, const Integer &e1,
                                    const Element &y, const Integer &e2) const;

    virtual void SimultaneousExponentiate(Element *results, const Element&,
                  const Integer *exponents, unsigned int exponentsCount) const;

    virtual const AbstractGroup& MultiplicativeGroup() const
        {return m_mg;}

private:
    class MultiplicativeGroupT : public AbstractGroup
    {
    public:
        const AbstractRing& GetRing() const
            {return *m_pRing;}

        bool Equal(const Element &a, const Element &b) const
            {return GetRing().Equal(a, b);}

        const Element& Identity() const
            {return GetRing().MultiplicativeIdentity();}

        const Element& Add(const Element &a, const Element &b) const
            {return GetRing().Multiply(a, b);}

        Element& Accumulate(Element &a, const Element &b) const
            {return a = GetRing().Multiply(a, b);}

        const Element& Inverse(const Element &a) const
            {return GetRing().MultiplicativeInverse(a);}

        const Element& Subtract(const Element &a, const Element &b) const
            {return GetRing().Divide(a, b);}

        Element& Reduce(Element &a, const Element &b) const
            {return a = GetRing().Divide(a, b);}

        const Element& Double(const Element &a) const
            {return GetRing().Square(a);}

        Element ScalarMultiply(const Element &a, const Integer &e) const
            {return GetRing().Exponentiate(a, e);}

        Element CascadeScalarMultiply(const Element &x, const Integer &e1,
                                     const Element &y, const Integer &e2) const
            {return GetRing().CascadeExponentiate(x, e1, y, e2);}

        void SimultaneousMultiply(Element *results, const Element &base,
                   const Integer *exponents, unsigned int exponentsCount) const
            {GetRing().SimultaneousExponentiate(results, base, exponents,
                                                exponentsCount);}

        const AbstractRing* m_pRing;
    };

    MultiplicativeGroupT m_mg;
};


// Abstract Euclidean Domain
class TAOCRYPT_NO_VTABLE AbstractEuclideanDomain
    : public AbstractRing
{
public:
    typedef Integer Element;

    virtual void DivisionAlgorithm(Element &r, Element &q, const Element &a,
                                   const Element &d) const =0;

    virtual const Element& Mod(const Element &a, const Element &b) const =0;
    virtual const Element& Gcd(const Element &a, const Element &b) const;

protected:
    mutable Element result;
};


// EuclideanDomainOf
class EuclideanDomainOf : public AbstractEuclideanDomain
{
public:
    typedef Integer Element;

    EuclideanDomainOf() {}

    bool Equal(const Element &a, const Element &b) const
        {return a==b;}

    const Element& Identity() const
        {return Element::Zero();}

    const Element& Add(const Element &a, const Element &b) const
        {return result = a+b;}

    Element& Accumulate(Element &a, const Element &b) const
        {return a+=b;}

    const Element& Inverse(const Element &a) const
        {return result = -a;}

    const Element& Subtract(const Element &a, const Element &b) const
        {return result = a-b;}

    Element& Reduce(Element &a, const Element &b) const
        {return a-=b;}

    const Element& Double(const Element &a) const
        {return result = a.Doubled();}

    const Element& MultiplicativeIdentity() const
        {return Element::One();}

    const Element& Multiply(const Element &a, const Element &b) const
        {return result = a*b;}

    const Element& Square(const Element &a) const
        {return result = a.Squared();}

    bool IsUnit(const Element &a) const
        {return a.IsUnit();}

    const Element& MultiplicativeInverse(const Element &a) const
        {return result = a.MultiplicativeInverse();}

    const Element& Divide(const Element &a, const Element &b) const
        {return result = a/b;}

    const Element& Mod(const Element &a, const Element &b) const
        {return result = a%b;}

    void DivisionAlgorithm(Element &r, Element &q, const Element &a,
                           const Element &d) const
        {Element::Divide(r, q, a, d);}

private:
    mutable Element result;
};



} // namespace

#endif // TAO_CRYPT_ALGEBRA_HPP
