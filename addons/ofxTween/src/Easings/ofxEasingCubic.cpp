#include "ofxEasingCubic.h"

ofxEasingCubic EasingCubic;

float ofxEasingCubic::easeIn (float t, float b, float c, float d) const {
	t /= d;

	return c * t * t * t + b;
}
float ofxEasingCubic::easeOut(float t, float b, float c, float d) const {
	t = t / d - 1;

	return c * (t * t * t + 1) + b;
}

float ofxEasingCubic::easeInOut(float t, float b, float c, float d) const {
	t /= d / 2;
	if(t < 1){
		return c / 2 * t * t * t + b;
	}

	t -= 2;

	return c / 2 * (t * t * t + 2) + b;
}
