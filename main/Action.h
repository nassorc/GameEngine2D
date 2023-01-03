#pragma once

#include <string>
// action class will store the type of action that the player wants the scene to perform
// DF = input -> gameEngine -> construct actions -> send to scene -> perform logic via doAction()

// two main variables
// - name (jump, shoot, etc)
// - type: START or END

// e.g. ("Right", "Start") - player will run right as if you held down the move right
//							 input button continuously
//                         - will only stop moving on ("right", "end")

class Action {
public:
	std::string m_name = "NONE";
	std::string m_type = "NONE";
	Action() {

	}
	Action(const std::string& name, const std::string& type)
		: m_name(name)
		, m_type(type)
	{

	}
	const std::string& name() const {
		return m_name;
	}
	const std::string& type() const {
		return m_type;

	}
	std::string toString() const {
		return m_type + " " + m_name;

	}
};