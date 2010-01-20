#ifndef PROJECTM_ALGORITHMS_HPP
#define PROJECTM_ALGORITHMS_HPP
#include <cassert>
/// A collection of algorithms generically written over map style containers
namespace Algorithms
{

  template <class TraverseFunctor, class Container>
  void traverse(Container & container)
  {

    TraverseFunctor functor;

    for (typename Container::iterator pos = container.begin(); pos != container.end(); ++pos)
    {
      assert(pos->second);
      functor(pos->second);
    }

  }


  template <class TraverseFunctor, class Container>
  void traverseVector(Container & container)
  {

    TraverseFunctor functor;

    for (typename Container::iterator pos = container.begin(); pos != container.end(); ++pos)
    {
      assert(*pos);
      functor(*pos);
    }

  }

  template <class TraverseFunctor, class Container>
  void traverse(Container & container, TraverseFunctor & functor)
  {

    for (typename Container::iterator pos = container.begin(); pos != container.end(); ++pos)
    {
      assert(pos->second);
      functor(pos->second);
    }

  }

  namespace TraverseFunctors
  {
    template <class Data>
    class DeleteFunctor
    {

    public:

      void operator() (Data * data)
      {
        assert(data);
        delete(data);
      }

    };
  }


}
#endif
