#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP
/// Code courtesy of: http://www.osix.net/modules/article/?id=464
 
template<typename kind> 
class RingBuffer { 
public:

	//static const unsigned long RING_BUFFER_SIZE = 1024;
	static const unsigned long RING_BUFFER_SIZE = 32768;
private:

    kind buffer[RING_BUFFER_SIZE]; 
    unsigned int current_element; 
public: 
    RingBuffer() : current_element(0) { 
    } 
 
    RingBuffer(const RingBuffer& old_ring_buf) { 
        memcpy(buffer, old_ring_buf.buffer, RING_BUFFER_SIZE*sizeof(kind)); 
        current_element = old_ring_buf.current_element; 
    } 
 
    RingBuffer operator = (const RingBuffer& old_ring_buf) { 
        memcpy(buffer, old_ring_buf.buffer, RING_BUFFER_SIZE*sizeof(kind)); 
        current_element = old_ring_buf.current_element; 
    } 
 
    ~RingBuffer() { } 
 
    void append(kind value) { 
        if (current_element == RING_BUFFER_SIZE) { 
            current_element = 0; 
        }
 
        buffer[current_element] = value; 
 
        ++current_element; 
    }
 

    kind back() {
	
	if (current_element == 0) {
            current_element = RING_BUFFER_SIZE;
        }
	--current_element;
 	return buffer[current_element];
    }

 
    kind forward() { 
        if(current_element >= RING_BUFFER_SIZE) { 
            current_element = 0; 
        } 
 
        ++current_element; 
        return( buffer[(current_element-1)] ); 
    } 
 
    int current() { 
        return (current_element % RING_BUFFER_SIZE); 
    }
}; 
#endif
