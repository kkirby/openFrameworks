#include "ofxEasingCirc.h"

ofxEasingCirc EasingCirc;

float ofxEasingCirc::easeIn (float t, float b, float c, float d) const {
	t /= d;

	return -c * (sqrt(1 - t * t) - 1) + b;
}
float ofxEasingCirc::easeOut(float t, float b, float c, float d) const {
	t = t / d - 1;

	return c * sqrt(1 - t * t) + b;
}

float ofxEasingCirc::easeInOut(float t, float b, float c, float d) const {
	t /= d / 2;
	if(t < 1){
		return c / 2 * (1 - sqrt(1 - t * t)) + b;
	}
	t -= 2;

	return c / 2 * (sqrt(1 - t * t) + 1) + b;
}
