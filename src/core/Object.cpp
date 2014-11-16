#include "Object.h"
#include "InitVisitor.h"
#include "JsonSerializerVisitor.h"
#include "JsonDeserializerVisitor.h"

namespace orm
{

Object::~Object()
{
}

void Object::init(bool omit)
{
	pkInitVisitor vis(omit);
	vis.visit(this);
}

String Object::toString(bool prettyFormatted) const
{
    JsonSerializerVisitor vis;
    vis.visit(const_cast<Object *>(this));
	if (prettyFormatted)
	{
		Json::StyledWriter writer;
        return String(writer.write(vis.rootValue()).c_str());
	}
	else
	{
        Json::FastWriter writer;
        return String(writer.write(vis.rootValue()).c_str());
	}
}

void Object::fromString(const String& s)
{
	Json::Reader reader;
	Json::Value root;
    if (!reader.parse(s.begin(), s.end(), root, false))
		throw std::invalid_argument("Json::Reader::parse failed");
    JsonDeserializerVisitor vis(root);
	vis.visit(this);
}

} // namespace pkapi
