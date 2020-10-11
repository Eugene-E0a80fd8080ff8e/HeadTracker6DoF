#pragma once

#include <memory>
#include <string>
#include <optional>


class PipeSender {

protected:

	PipeSender() {}

	PipeSender(const PipeSender&) = delete; 
	PipeSender& operator=(PipeSender const&) = delete;


public:

	virtual ~PipeSender() {};

	// openPipe returns null upon success
	// an error message otherwise
	virtual std::optional<std::string> openPipe() = 0;

	// x,y,z is coordinates of detected head
	// x for camera's sideways dimension, y is for vertical, z towards the user
	// a,b,c is a Rodrigues' rotation vector
	virtual void send(float x, float y, float z
					, float a, float b, float c) = 0;

	virtual void close() = 0 ;


};

std::unique_ptr<PipeSender> factory_PipeSender();
