//
// Created by Leonard Chan on 3/11/25.
//

#ifndef TIMER_H
#define TIMER_H

class Timer {
  public:
	Timer() { reset(); }

	void reset() { start = std::chrono::high_resolution_clock::now(); }

	float elapsed() {
		return std::chrono::duration_cast<std::chrono::nanoseconds>(
		           std::chrono::high_resolution_clock::now() - start)
		           .count() *
		       0.001f * 0.001f * 0.001f;
	}

	float elapsedMillis() { return elapsed() * 1000.0f; }

  private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

class ScopedTimer {
  public:
	ScopedTimer(const std::string &name) : name(name) {}
	~ScopedTimer() {
		float time = timer.elapsedMillis();
		std::cout << "[TIMER] " << name << " - " << time << "ms\n";
	}

  private:
	std::string name;
	Timer timer;
};

#endif // TIMER_H
