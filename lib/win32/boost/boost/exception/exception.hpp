//Copyright (c) 2006-2008 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef UUID_274DA366004E11DCB1DDFE2E56D89593
#define UUID_274DA366004E11DCB1DDFE2E56D89593

namespace
boost
    {
    namespace
    exception_detail
        {
        template <class T>
        class
        refcount_ptr
            {
            public:

            refcount_ptr():
                px_(0)
                {
                }

            ~refcount_ptr()
                {
                release();
                }

            refcount_ptr( refcount_ptr const & x ):
                px_(x.px_)
                {
                add_ref();
                }

            refcount_ptr &
            operator=( refcount_ptr const & x )
                {
                adopt(x.px_);
                return *this;
                }

            void
            adopt( T * px )
                {
                release();
                px_=px;
                add_ref();
                }

            T *
            get() const
                {
                return px_;
                }

            private:

            T * px_;

            void
            add_ref()
                {
                if( px_ )
                    px_->add_ref();
                }

            void
            release()
                {
                if( px_ )
                    px_->release();
                }
            };
        }

    ////////////////////////////////////////////////////////////////////////

    template <class Tag,class T>
    class error_info;

    typedef error_info<struct tag_throw_function,char const *> throw_function;
    typedef error_info<struct tag_throw_file,char const *> throw_file;
    typedef error_info<struct tag_throw_line,int> throw_line;

    template <>
    class
    error_info<tag_throw_function,char const *>
        {
        public:
        typedef char const * value_type;
        value_type v_;
        explicit
        error_info( value_type v ):
            v_(v)
            {
            }
        };

    template <>
    class
    error_info<tag_throw_file,char const *>
        {
        public:
        typedef char const * value_type;
        value_type v_;
        explicit
        error_info( value_type v ):
            v_(v)
            {
            }
        };

    template <>
    class
    error_info<tag_throw_line,int>
        {
        public:
        typedef int value_type;
        value_type v_;
        explicit
        error_info( value_type v ):
            v_(v)
            {
            }
        };

    template <class E,class Tag,class T>
    E const & operator<<( E const &, error_info<Tag,T> const & );

    class exception;

    template <class>
    class shared_ptr;

    namespace
    exception_detail
        {
        class error_info_base;
        struct type_info_;

        struct
        error_info_container
            {
            virtual char const * diagnostic_information() const = 0;
            virtual shared_ptr<error_info_base const> get( type_info_ const & ) const = 0;
            virtual void set( shared_ptr<error_info_base const> const &, type_info_ const & ) = 0;
            virtual void add_ref() const = 0;
            virtual void release() const = 0;

            protected:

            virtual
            ~error_info_container() throw()
                {
                }
            };

        template <class>
        struct get_info;

        template <>
        struct get_info<throw_function>;

        template <>
        struct get_info<throw_file>;

        template <>
        struct get_info<throw_line>;

        char const * get_diagnostic_information( exception const & );
        }

    class
    exception
        {
        protected:

        exception():
            throw_function_(0),
            throw_file_(0),
            throw_line_(-1)
            {
            }

#ifdef __HP_aCC
        //On HP aCC, this protected copy constructor prevents throwing boost::exception.
        //On all other platforms, the same effect is achieved by the pure virtual destructor.
        exception( exception const & x ) throw():
            data_(x.data_),
            throw_function_(x.throw_function_),
            throw_file_(x.throw_file_),
            throw_line_(x.throw_line_)
            {
            }
#endif

        virtual ~exception() throw()
#ifndef __HP_aCC
            = 0 //Workaround for HP aCC, =0 incorrectly leads to link errors.
#endif
            ;

        private:

        template <class E>
        friend
        E const &
        operator<<( E const & x, throw_function const & y )
            {
            x.throw_function_=y.v_;
            return x;
            }

        template <class E>
        friend
        E const &
        operator<<( E const & x, throw_file const & y )
            {
            x.throw_file_=y.v_;
            return x;
            }

        template <class E>
        friend
        E const &
        operator<<( E const & x, throw_line const & y )
            {
            x.throw_line_=y.v_;
            return x;
            }

        friend char const * exception_detail::get_diagnostic_information( exception const & );

        template <class E,class Tag,class T>
        friend E const & operator<<( E const &, error_info<Tag,T> const & );

        template <class>
        friend struct exception_detail::get_info;
        friend struct exception_detail::get_info<throw_function>;
        friend struct exception_detail::get_info<throw_file>;
        friend struct exception_detail::get_info<throw_line>;

        mutable exception_detail::refcount_ptr<exception_detail::error_info_container> data_;
        mutable char const * throw_function_;
        mutable char const * throw_file_;
        mutable int throw_line_;
        };

    inline
    exception::
    ~exception() throw()
        {
        }

    ////////////////////////////////////////////////////////////////////////

    namespace
    exception_detail
        {
        template <class T>
        struct
        error_info_injector:
            public T,
            public exception
            {
            explicit
            error_info_injector( T const & x ):
                T(x)
                {
                }

            ~error_info_injector() throw()
                {
                }
            };

        struct large_size { char c[256]; };
        large_size dispatch( exception * );

        struct small_size { };
        small_size dispatch( void * );

        template <class,int>
        struct enable_error_info_helper;

        template <class T>
        struct
        enable_error_info_helper<T,sizeof(large_size)>
            {
            typedef T type;
            };

        template <class T>
        struct
        enable_error_info_helper<T,sizeof(small_size)>
            {
            typedef error_info_injector<T> type;
            };

        template <class T>
        struct
        enable_error_info_return_type
            {
            typedef typename enable_error_info_helper<T,sizeof(dispatch((T*)0))>::type type;
            };
        }

    template <class T>
    inline
    typename
    exception_detail::enable_error_info_return_type<T>::type
    enable_error_info( T const & x )
        {
        typedef typename exception_detail::enable_error_info_return_type<T>::type rt;
        return rt(x);
        }

    ////////////////////////////////////////////////////////////////////////

    namespace
    exception_detail
        {
        class
        clone_base
            {
            public:

            virtual clone_base const * clone() const = 0;
            virtual void rethrow() const = 0;

            virtual
            ~clone_base() throw()
                {
                }
            };

        inline
        void
        copy_boost_exception( exception * a, exception const * b )
            {
            *a = *b;
            }

        inline
        void
        copy_boost_exception( void *, void const * )
            {
            }

        template <class T>
        class
        clone_impl:
            public T,
            public clone_base
            {
            public:

            explicit
            clone_impl( T const & x ):
                T(x)
                {
                copy_boost_exception(this,&x);
                }

            ~clone_impl() throw()
                {
                }

            private:

            clone_base const *
            clone() const
                {
                return new clone_impl(*this);
                }

            void
            rethrow() const
                {
                throw*this;
                }
            };
        }

    template <class T>
    inline
    exception_detail::clone_impl<T>
    enable_current_exception( T const & x )
        {
        return exception_detail::clone_impl<T>(x);
        }
    }

#endif
