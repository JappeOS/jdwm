#include "surface_buffer_chain.hpp"

bool is_main_thread() { return true; }

int main() {
	std::shared_ptr<int> first_frame;
	std::shared_ptr<int> second_frame;
	std::weak_ptr<int> first_buffer;
	std::weak_ptr<int> second_buffer;
	{
		SurfaceBufferChain<int> chain;
		assert(!chain.start_read());
		chain.commit_buffer(std::make_shared<int>(1));
		first_frame = chain.start_read();
		first_buffer = first_frame;
		chain.commit_buffer(std::make_shared<int>(2));
		second_frame = chain.start_read();
		second_buffer = second_frame;
		// Replacing the current frame must not release an image Flutter still uses.
		assert(!first_buffer.expired());
		assert(*first_frame == 1);
		assert(*second_frame == 2);
		chain.commit_buffer(std::make_shared<int>(3));
		first_frame.reset();
		assert(first_buffer.expired());
		// A delayed release for frame 1 must not release frame 2.
		assert(!second_buffer.expired());
	}
	// A texture may outlive its surface/unregistration.
	assert(!second_buffer.expired());
	second_frame.reset();
	assert(second_buffer.expired());
}
