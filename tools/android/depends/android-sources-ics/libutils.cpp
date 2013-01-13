/*
 *      Copyright (C) 2010-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/RefBase.h"
#include "utils/String16.h"
#include "utils/String8.h"
#include "utils/VectorImpl.h"
#include "utils/Unicode.h"

extern "C" {

int strzcmp16(const char16_t *s1, size_t n1, const char16_t *s2, size_t n2)
{
  return 0;
}

}

namespace android {
RefBase::RefBase() : mRefs(0)
{
}

RefBase::~RefBase()
{
}

void RefBase::incStrong(const void *id) const
{
}

void RefBase::decStrong(const void *id) const
{
}

void RefBase::onFirstRef()
{
}

void RefBase::onLastStrongRef(const void* id)
{
}

bool RefBase::onIncStrongAttempted(uint32_t flags, const void* id)
{
  return false;
}

void RefBase::onLastWeakRef(void const* id)
{
}

String16::String16()
{
}

String16::String16(String16 const&)
{
}

String16::String16(char const*)
{
}

String16::~String16()
{
}

String8::String8()
{
}

String8::~String8()
{
}

VectorImpl::VectorImpl(size_t itemSize, uint32_t flags)
    : mFlags(flags), mItemSize(itemSize)
{
}

VectorImpl::VectorImpl(const VectorImpl& rhs)
    : mFlags(rhs.mFlags), mItemSize(rhs.mItemSize)
{
}

VectorImpl::~VectorImpl()
{
}

VectorImpl& VectorImpl::operator = (const VectorImpl& rhs)
{
}

void* VectorImpl::editArrayImpl()
{
}

size_t VectorImpl::capacity() const
{
  return 0;
}

ssize_t VectorImpl::insertVectorAt(const VectorImpl& vector, size_t index)
{
  return 0;
}

ssize_t VectorImpl::appendVector(const VectorImpl& vector)
{
  return 0;
}

ssize_t VectorImpl::insertArrayAt(const void* array, size_t index, size_t length)
{
  return 0;
}

ssize_t VectorImpl::appendArray(const void* array, size_t length)
{
  return 0;
}

ssize_t VectorImpl::insertAt(size_t index, size_t numItems)
{
  return 0;
}

ssize_t VectorImpl::insertAt(const void* item, size_t index, size_t numItems)
{
  return 0;
}

static int sortProxy(const void* lhs, const void* rhs, void* func)
{
  return 0;
}

status_t VectorImpl::sort(VectorImpl::compar_t cmp)
{
  return 0;
}

status_t VectorImpl::sort(VectorImpl::compar_r_t cmp, void* state)
{
  return 0;
}

void VectorImpl::pop()
{
}

void VectorImpl::push()
{
}

void VectorImpl::push(const void* item)
{
}

ssize_t VectorImpl::add()
{
  return 0;
}

ssize_t VectorImpl::add(const void* item)
{
  return 0;
}

ssize_t VectorImpl::replaceAt(size_t index)
{
  return 0;
}

ssize_t VectorImpl::replaceAt(const void* prototype, size_t index)
{
  return 0;
}

ssize_t VectorImpl::removeItemsAt(size_t index, size_t count)
{
  return 0;
}

void VectorImpl::finish_vector()
{
}

void VectorImpl::clear()
{
}

void* VectorImpl::editItemLocation(size_t index)
{
  return 0;
}

const void* VectorImpl::itemLocation(size_t index) const
{
  return 0;
}

ssize_t VectorImpl::setCapacity(size_t new_capacity)
{
  return 0;
}

void VectorImpl::release_storage()
{
}

void* VectorImpl::_grow(size_t where, size_t amount)
{
  return 0;
}

void VectorImpl::_shrink(size_t where, size_t amount)
{
}

size_t VectorImpl::itemSize() const {
  return 0;
}

void VectorImpl::_do_construct(void* storage, size_t num) const
{
}

void VectorImpl::_do_destroy(void* storage, size_t num) const
{
}

void VectorImpl::_do_copy(void* dest, const void* from, size_t num) const
{
}

void VectorImpl::_do_splat(void* dest, const void* item, size_t num) const {
}

void VectorImpl::_do_move_forward(void* dest, const void* from, size_t num) const {
}

void VectorImpl::_do_move_backward(void* dest, const void* from, size_t num) const {
}

void VectorImpl::reservedVectorImpl1() { }
void VectorImpl::reservedVectorImpl2() { }
void VectorImpl::reservedVectorImpl3() { }
void VectorImpl::reservedVectorImpl4() { }
void VectorImpl::reservedVectorImpl5() { }
void VectorImpl::reservedVectorImpl6() { }
void VectorImpl::reservedVectorImpl7() { }
void VectorImpl::reservedVectorImpl8() { }

/*****************************************************************************/

SortedVectorImpl::SortedVectorImpl(size_t itemSize, uint32_t flags)
    : VectorImpl(itemSize, flags)
{
}

SortedVectorImpl::SortedVectorImpl(const VectorImpl& rhs)
: VectorImpl(rhs)
{
}

SortedVectorImpl::~SortedVectorImpl()
{
}

ssize_t SortedVectorImpl::indexOf(const void* item) const
{
  return 0;
}

size_t SortedVectorImpl::orderOf(const void* item) const
{
  return 0;
}

ssize_t SortedVectorImpl::_indexOrderOf(const void* item, size_t* order) const
{
  return 0;
}

ssize_t SortedVectorImpl::add(const void* item)
{
  return 0;
}

ssize_t SortedVectorImpl::merge(const VectorImpl& vector)
{
  return 0;
}

ssize_t SortedVectorImpl::merge(const SortedVectorImpl& vector)
{
  return 0;
}

ssize_t SortedVectorImpl::remove(const void* item)
{
  return 0;
}

void SortedVectorImpl::reservedSortedVectorImpl1() { };
void SortedVectorImpl::reservedSortedVectorImpl2() { };
void SortedVectorImpl::reservedSortedVectorImpl3() { };
void SortedVectorImpl::reservedSortedVectorImpl4() { };
void SortedVectorImpl::reservedSortedVectorImpl5() { };
void SortedVectorImpl::reservedSortedVectorImpl6() { };
void SortedVectorImpl::reservedSortedVectorImpl7() { };
void SortedVectorImpl::reservedSortedVectorImpl8() { };

}
