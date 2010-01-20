// memory.cpp
#include "../../include/lock.hpp"     // locking
#include <new>          // std::bad_alloc
#include <cstdlib>      // malloc
#include <cstring>      // memset
#include <fstream>      // ofstream
#include <sstream>      // stringstream
#include <cassert>      // assert
#include <iomanip>      // setiosflags

/*********************************************************************

To use MemoryTracker merely add this file to your project
No need to instantiate anything

If your app is multi threaded define MULTI_THREADED

*********************************************************************/


// locals
namespace {

class MemoryTracker {
    std::ofstream log_;
public:
    MemoryTracker();
    ~MemoryTracker();
private:
    MemoryTracker(const MemoryTracker&);             // hide copy
    MemoryTracker& operator=(const MemoryTracker&);  // and assign

    void LogStats();
};


struct alloc_node {
    alloc_node* left_;
    alloc_node* right_;
  
    alloc_node() : left_(0), right_(0) {}
};


alloc_node* Root = 0;

size_t Allocs    = 0;
size_t DeAllocs  = 0;
size_t Bytes     = 0;


struct size_tracker {
    size_t size_;
    size_t count_;
};

size_tracker sizes[] = 
{
    {0,0},
    {2,0},
    {4,0},
    {8,0},
    {16,0},
    {32,0},
    {64,0},
    {128,0},
    {256,0},
    {512,0},
    {1024,0},
    {2048,0},
    {4096,0},
    {8192,0},
};

const size_t size_elements(sizeof(sizes) / sizeof(size_tracker));

bool Tracking(false);

using   yaSSL::Mutex;
typedef Mutex::Lock Lock;

Mutex mutex;

MemoryTracker theTracker;


bool lookup(alloc_node*& find, void* key, alloc_node*& prev)
{
    bool found(false);

    while (find) {
        if (find == key) {
            found = true;
            break;
        }
        prev = find;
        if (key < find)
            find = find->left_;
        else
            find = find->right_;
    }
    return found;
}


// iterative insert
void insert(alloc_node* entry)
{
    if (!Root) {
        Root = entry;
        return;
    }
       
    alloc_node* tmp  = Root;
    alloc_node* prev = 0;

    if (lookup(tmp, entry, prev)) 
        assert(0); // duplicate

    if (entry < prev)
        prev->left_  = entry;
    else
        prev->right_ = entry;
}


alloc_node* predecessorSwap(alloc_node* del)
{
    alloc_node* pred = del->left_;
    alloc_node* predPrev = del;

    while (pred->right_) {
        predPrev = pred;
        pred = pred->right_;
    }
    if (predPrev == del)
        predPrev->left_  = pred->left_;
    else
        predPrev->right_ = pred->left_;

    pred->left_  = del->left_;
    pred->right_ = del->right_;

    return pred;
}


// iterative remove
void remove(void* ptr)
{
    alloc_node* del  = Root;
    alloc_node* prev = 0;
    alloc_node* replace = 0;

    if ( lookup(del, ptr, prev) == false)
        assert(0); // oops, not there

    if (del->left_ && del->right_)          // two children
        replace = predecessorSwap(del);
    else if (!del->left_ && !del->right_)   // no children
        replace = 0;
    else                                    // one child
        replace = (del->left_) ? del->left_ : del->right_;

    if (del == Root)
        Root = replace;
    else if (prev->left_ == del)
        prev->left_  = replace;
    else
        prev->right_ = replace;
}


typedef void (*fp)(alloc_node*, void*);

void applyInOrder(alloc_node* root, fp f, void* arg)
{
    if (root == 0)
        return;
    
    applyInOrder(root->left_,  f, arg);
    f(root, arg);
    applyInOrder(root->right_, f, arg);
}


void show(alloc_node* ptr, void* arg)
{
    std::ofstream* log = static_cast<std::ofstream*>(arg);
    *log << ptr << '\n';
}


MemoryTracker::MemoryTracker() : log_("memory.log")
{
#ifdef __GNUC__
    // Force pool allocator to cleanup at exit
    setenv("GLIBCPP_FORCE_NEW", "1", 0);
#endif

#ifdef _MSC_VER
    // msvc6 needs to create Facility for ostream before main starts, otherwise
    // if another ostream is created and destroyed in main scope, log stats
    // will access a dead Facility reference (std::numput)
    int msvcFac = 6;
    log_ << "MSVC " << msvcFac << "workaround" << std::endl; 
#endif


    Tracking = true;
}


MemoryTracker::~MemoryTracker()
{
    // stop tracking before log (which will alloc on output)
    Tracking = false;
    LogStats();

    //assert(Allocs == DeAllocs);
    //assert(Root == 0);
}


void MemoryTracker::LogStats()
{
    log_ << "Number of Allocs:     " << Allocs    << '\n';
    log_ << "Number of DeAllocs:   " << DeAllocs  << '\n';
    log_ << "Number of bytes used: " << Bytes     << '\n';

    log_ << "Alloc size table:\n";
    log_ << " Bytes " << '\t' << "   Times\n";

    for (size_t i = 0; i < size_elements; ++i) {
        log_ << " " << sizes[i].size_  << "  " << '\t';
        log_ << std::setiosflags(std::ios::right) << std::setw(8);
        log_ << sizes[i].count_ << '\n';
    }

    if (Allocs != DeAllocs) {
        log_<< "Showing new'd allocs with no deletes" << '\n';
        applyInOrder(Root, show, &log_);
    }
    log_.flush();
}


// return power of 2 up to size_tracker elements
size_t powerOf2(size_t sz)
{
    size_t shifts = 0;

    if (sz)
        sz -= 1;
    else
        return 0;
	   
    while (sz) {
        sz >>= 1;
        ++shifts;
    }

    return shifts < size_elements ? shifts : size_elements;
}


} // namespace local


void* operator new(size_t sz)
{
    // put alloc node in front of requested memory
    void* ptr = malloc(sz + sizeof(alloc_node));
    if (ptr) {
        if (Tracking) {
            Lock l(mutex);
            ++Allocs;
            Bytes += sz;
            ++sizes[powerOf2(sz)].count_;
            insert(new (ptr) alloc_node);
        }
        return static_cast<char*>(ptr) + sizeof(alloc_node);
    }
    else
        assert(0);
}


void operator delete(void* ptr)
{
    if (ptr) {
        ptr = static_cast<char*>(ptr) - sizeof(alloc_node);  // correct offset
        if (Tracking) {
            Lock l(mutex);
            ++DeAllocs;
            remove(ptr);
        }
        free(ptr);
    }
}


void* operator new[](size_t sz)
{
    return ::operator new(sz);
}


void operator delete[](void* ptr)
{
    ::operator delete(ptr);
}
