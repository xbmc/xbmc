/* template_instnt.cpp                               
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


/*  Explicit template instantiation requests 
 */


#include "runtime.hpp"
#include "handshake.hpp"
#include "yassl_int.hpp"
#include "crypto_wrapper.hpp"
#include "hmac.hpp"
#include "md5.hpp"
#include "sha.hpp"
#include "ripemd.hpp"
#include "openssl/ssl.h"

#ifdef HAVE_EXPLICIT_TEMPLATE_INSTANTIATION

namespace mySTL {
template class list<unsigned char*>;
template yaSSL::del_ptr_zero for_each(mySTL::list<unsigned char*>::iterator, mySTL::list<unsigned char*>::iterator, yaSSL::del_ptr_zero);
template pair<int, yaSSL::Message* (*)()>* uninit_copy<mySTL::pair<int, yaSSL::Message* (*)()>*, mySTL::pair<int, yaSSL::Message* (*)()>*>(mySTL::pair<int, yaSSL::Message* (*)()>*, mySTL::pair<int, yaSSL::Message* (*)()>*, mySTL::pair<int, yaSSL::Message* (*)()>*);
template pair<int, yaSSL::HandShakeBase* (*)()>* uninit_copy<mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*, mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*>(mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*, mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*, mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*);
template void destroy<mySTL::pair<int, yaSSL::Message* (*)()>*>(mySTL::pair<int, yaSSL::Message* (*)()>*, mySTL::pair<int, yaSSL::Message* (*)()>*);
template void destroy<mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*>(mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*, mySTL::pair<int, yaSSL::HandShakeBase* (*)()>*);
template pair<int, yaSSL::ServerKeyBase* (*)()>* uninit_copy<mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*>(mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*);
template void destroy<mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*>(mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ServerKeyBase* (*)()>*);
template pair<int, yaSSL::ClientKeyBase* (*)()>* uninit_copy<mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*>(mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*);
template class list<TaoCrypt::Signer*>;
template class list<yaSSL::SSL_SESSION*>;
template class list<yaSSL::input_buffer*>;
template class list<yaSSL::output_buffer*>;
template class list<yaSSL::x509*>;
template class list<yaSSL::Digest*>;
template class list<yaSSL::BulkCipher*>;
template void destroy<mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*>(mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*, mySTL::pair<int, yaSSL::ClientKeyBase* (*)()>*);
template yaSSL::del_ptr_zero for_each<mySTL::list<TaoCrypt::Signer*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<TaoCrypt::Signer*>::iterator, mySTL::list<TaoCrypt::Signer*>::iterator, yaSSL::del_ptr_zero);
template yaSSL::del_ptr_zero for_each<mySTL::list<yaSSL::SSL_SESSION*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<yaSSL::SSL_SESSION*>::iterator, mySTL::list<yaSSL::SSL_SESSION*>::iterator, yaSSL::del_ptr_zero);
template yaSSL::del_ptr_zero for_each<mySTL::list<yaSSL::input_buffer*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<yaSSL::input_buffer*>::iterator, mySTL::list<yaSSL::input_buffer*>::iterator, yaSSL::del_ptr_zero);
template yaSSL::del_ptr_zero for_each<mySTL::list<yaSSL::output_buffer*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<yaSSL::output_buffer*>::iterator, mySTL::list<yaSSL::output_buffer*>::iterator, yaSSL::del_ptr_zero);
template yaSSL::del_ptr_zero for_each<mySTL::list<yaSSL::x509*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<yaSSL::x509*>::iterator, mySTL::list<yaSSL::x509*>::iterator, yaSSL::del_ptr_zero);
template yaSSL::del_ptr_zero for_each<mySTL::list<yaSSL::Digest*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<yaSSL::Digest*>::iterator, mySTL::list<yaSSL::Digest*>::iterator, yaSSL::del_ptr_zero);
template yaSSL::del_ptr_zero for_each<mySTL::list<yaSSL::BulkCipher*>::iterator, yaSSL::del_ptr_zero>(mySTL::list<yaSSL::BulkCipher*>::iterator, mySTL::list<yaSSL::BulkCipher*>::iterator, yaSSL::del_ptr_zero);
template bool list<yaSSL::ThreadError>::erase(list<yaSSL::ThreadError>::iterator);
template void list<yaSSL::ThreadError>::push_back(yaSSL::ThreadError);
template void list<yaSSL::ThreadError>::pop_front();
template void list<yaSSL::ThreadError>::pop_back();
template list<yaSSL::ThreadError>::~list();
template pair<int, yaSSL::Message* (*)()>* GetArrayMemory<pair<int, yaSSL::Message* (*)()> >(size_t);
template void FreeArrayMemory<pair<int, yaSSL::Message* (*)()> >(pair<int, yaSSL::Message* (*)()>*);
template pair<int, yaSSL::HandShakeBase* (*)()>* GetArrayMemory<pair<int, yaSSL::HandShakeBase* (*)()> >(size_t);
template void FreeArrayMemory<pair<int, yaSSL::HandShakeBase* (*)()> >(pair<int, yaSSL::HandShakeBase* (*)()>*);
template pair<int, yaSSL::ServerKeyBase* (*)()>* GetArrayMemory<pair<int, yaSSL::ServerKeyBase* (*)()> >(size_t);
template void FreeArrayMemory<pair<int, yaSSL::ServerKeyBase* (*)()> >(pair<int, yaSSL::ServerKeyBase* (*)()>*);
template pair<int, yaSSL::ClientKeyBase* (*)()>* GetArrayMemory<pair<int, yaSSL::ClientKeyBase* (*)()> >(size_t);
template void FreeArrayMemory<pair<int, yaSSL::ClientKeyBase* (*)()> >(pair<int, yaSSL::ClientKeyBase* (*)()>*);
}

namespace yaSSL {
template void ysDelete<SSL_CTX>(yaSSL::SSL_CTX*);
template void ysDelete<SSL>(yaSSL::SSL*);
template void ysDelete<BIGNUM>(yaSSL::BIGNUM*);
template void ysDelete<unsigned char>(unsigned char*);
template void ysDelete<DH>(yaSSL::DH*);
template void ysDelete<TaoCrypt::Signer>(TaoCrypt::Signer*);
template void ysDelete<SSL_SESSION>(yaSSL::SSL_SESSION*);
template void ysDelete<input_buffer>(input_buffer*);
template void ysDelete<output_buffer>(output_buffer*);
template void ysDelete<x509>(x509*);
template void ysDelete<Auth>(Auth*);
template void ysDelete<HandShakeBase>(HandShakeBase*);
template void ysDelete<ServerKeyBase>(ServerKeyBase*);
template void ysDelete<ClientKeyBase>(ClientKeyBase*);
template void ysDelete<SSL_METHOD>(SSL_METHOD*);
template void ysDelete<DiffieHellman>(DiffieHellman*);
template void ysDelete<BulkCipher>(BulkCipher*);
template void ysDelete<Digest>(Digest*);
template void ysDelete<X509>(X509*);
template void ysDelete<Message>(Message*);
template void ysDelete<sslFactory>(sslFactory*);
template void ysDelete<Sessions>(Sessions*);
template void ysDelete<Errors>(Errors*);
template void ysArrayDelete<unsigned char>(unsigned char*);
template void ysArrayDelete<char>(char*);

template int min<int>(int, int);
template uint16 min<uint16>(uint16, uint16);
template unsigned int min<unsigned int>(unsigned int, unsigned int);
template unsigned long min<unsigned long>(unsigned long, unsigned long);
}

#endif // HAVE_EXPLICIT_TEMPLATE_INSTANTIATION

