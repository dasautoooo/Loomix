//
// Created by Leonard Chan on 3/10/25.
//

#ifndef LAYER_H
#define LAYER_H

class Layer {
  public:
	virtual ~Layer() = default;

	virtual void onAttach() {}
	virtual void onDetach() {}

	virtual void onUpdate(float ts) {}
	virtual void onUIRender() {}
};

#endif // LAYER_H
