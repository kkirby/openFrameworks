#include "ofxEasingBack.h"

ofxEasingBack EasingBack;

float ofxEasingBack::easeIn (float t, float b, float c, float d) const {
	float s = 1.70158f;
	t /= d;
	float postFix = t;

	return c * postFix * t * ((s + 1) * t - s) + b;
}
float ofxEasingBack::easeOut(float t, float b, float c, float d) const {
	float s = 1.70158f;
	t = t / d - 1;

	return c * (t * t * ((s + 1) * t + s) + 1) + b;
}

float ofxEasingBack::easeInOut(float t, float b, float c, float d) const {
	float s = 1.70158f;
	t /= d / 2;
	if(t < 1){
		s *= (1.525f);

		return c / 2 * (t * t * ((s + 1) * t - s)) + b;
	}
	float postFix = t -= 2;

	s *= (1.525f);

	return c / 2 * ((postFix) * t * ((s + 1) * t + s) + 2) + b;
}
