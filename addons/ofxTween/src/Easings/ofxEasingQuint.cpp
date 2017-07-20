#include "ofxEasingQuint.h"

ofxEasingQuint EasingQuint;

float ofxEasingQuint::easeIn (float t, float b, float c, float d)  const {
	t /= d;

	return c * t * t * t * t * t + b;
}
float ofxEasingQuint::easeOut(float t, float b, float c, float d)  const {
	t = t / d - 1;

	return c * (t * t * t * t * t + 1) + b;
}

float ofxEasingQuint::easeInOut(float t, float b, float c, float d)  const {
	t /= d / 2;
	if(t < 1){
		return c / 2 * t * t * t * t * t + b;
	}
	t -= 2;

	return c / 2 * (t * t * t * t * t + 2) + b;
}
