#pragma once

#include <string>

/** UpdateMessage stores information for a message
  * about the status of update installation sent
  * between threads.
  */
class UpdateMessage
{
	public:
		enum Type
		{
			UpdateFailed,
			UpdateProgress,
			UpdateFinished
		};

		UpdateMessage(void* receiver, Type type)
		{
			init(receiver,type);
		}

		UpdateMessage(Type type)
		{
			init(0,type);
		}

		void* receiver;
		Type type;
		std::string message;
		int progress;

	private:
		void init(void* receiver, Type type)
		{
			this->progress = 0;
			this->receiver = receiver;
			this->type = type;
		}
};

