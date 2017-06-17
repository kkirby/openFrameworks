#pragma once

#include "jsonxx.h"
#include "ofMain.h"

#define OFX_JSONXX_BEGIN_NAMESPACE namespace ofx { namespace Jsonxx {
#define OFX_JSONXX_END_NAMESPACE } }

OFX_JSONXX_BEGIN_NAMESPACE

using namespace jsonxx;
using namespace std;

// load

inline bool loadFromString(Object& out, const string& json)
{
	return out.parse(json);
}

inline bool loadFromString(Array& out, const string& json)
{
	return out.parse(json);
}

inline bool loadFromFile(Object& out, const string& path)
{
	if (!ofFile::doesFileExist(path)) return false;
	
	ofBuffer buf = ofBufferFromFile(path);
	return loadFromString(out, buf.getText());
}

inline bool loadFromFile(Array& out, const string& path)
{
	if (!ofFile::doesFileExist(path)) return false;
	
	ofBuffer buf = ofBufferFromFile(path);
	return loadFromString(out, buf.getText());
}

// save

inline bool saveToString(Object& in, string& out)
{
	out = in.json();
	return true;
}

inline bool saveToString(Array& in, string& out)
{
	out = in.json();
	return true;
}

inline bool saveToFile(Object& in, const string& path)
{
	ofBuffer buf = in.json();
	return ofBufferToFile(path, buf);
}

inline bool saveToFile(Array& in, const string& path)
{
	ofBuffer buf = in.json();
	return ofBufferToFile(path, buf);
}

//

#define DEFINE_ACCESSOR(TYPE, JSON_TYPE) \
inline bool get(const Object& json, const string& key, TYPE &out) { \
	if (!json.has<JSON_TYPE>(key)) return false; \
	out = json.get<JSON_TYPE>(key); \
	return true; \
} \
inline TYPE get(const Object& json, const string& key, const TYPE& fallback) { \
	return json.get<JSON_TYPE>(key, fallback); \
} \
inline bool set(Object& json, const string& key, TYPE in) { \
	json << key << (JSON_TYPE)in; \
	return true; \
}

DEFINE_ACCESSOR(float, Number);
DEFINE_ACCESSOR(double, Number);
DEFINE_ACCESSOR(int, Number);
DEFINE_ACCESSOR(unsigned int, Number);
DEFINE_ACCESSOR(char, Number);
DEFINE_ACCESSOR(unsigned char, Number);
DEFINE_ACCESSOR(short, Number);
DEFINE_ACCESSOR(unsigned short, Number);
DEFINE_ACCESSOR(long, Number);
DEFINE_ACCESSOR(unsigned long, Number);
DEFINE_ACCESSOR(bool, jsonxx::Boolean);

DEFINE_ACCESSOR(string, string);
DEFINE_ACCESSOR(Object, Object);
DEFINE_ACCESSOR(Array, Array);

#undef DEFINE_ACCESSOR

//

OFX_JSONXX_END_NAMESPACE

namespace ofxJsonxx = ofx::Jsonxx;


typedef ofxJsonxx::Object JsonObject;
typedef ofxJsonxx::Array JsonArray;
typedef ofxJsonxx::Number JsonNumber;
typedef ofxJsonxx::Boolean JsonBoolean;

template <typename T> JsonArray jsonConvertToArray(T input){
	JsonArray jsonArray;
	for(auto& value : input){
		jsonArray << value;
	}
	return jsonArray;
}

#define jsonSet ofxJsonxx::set