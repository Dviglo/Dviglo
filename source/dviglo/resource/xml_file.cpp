// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) the Dviglo project
// License: MIT

#include "../core/context.h"
#include "../core/profiler.h"
#include "../io/deserializer.h"
#include "../io/log.h"
#include "../io/memory_buffer.h"
#include "../io/vector_buffer.h"
#include "resource_cache.h"
#include "xml_file.h"

#include <pugixml.hpp>

#include "../common/debug_new.h"

#include <memory>

using namespace std;

namespace dviglo
{

/// XML writer for pugixml.
class XMLWriter : public pugi::xml_writer
{
public:
    /// Construct.
    explicit XMLWriter(Serializer& dest) :
        dest_(dest),
        success_(true)
    {
    }

    /// Write bytes to output.
    void write(const void* data, size_t size) override
    {
        if (dest_.Write(data, (unsigned)size) != size)
            success_ = false;
    }

    /// Destination serializer.
    Serializer& dest_;
    /// Success flag.
    bool success_;
};

XmlFile::XmlFile() :
    document_(new pugi::xml_document())
{
}

XmlFile::~XmlFile() = default;

void XmlFile::register_object()
{
    DV_CONTEXT->RegisterFactory<XmlFile>();
}

bool XmlFile::begin_load(Deserializer& source)
{
    unsigned dataSize = source.GetSize();
    if (!dataSize && !source.GetName().Empty())
    {
        DV_LOGERROR("Zero sized XML data in " + source.GetName());
        return false;
    }

    unique_ptr<byte[]> buffer(new byte[dataSize]);
    if (source.Read(buffer.get(), dataSize) != dataSize)
        return false;

    if (!document_->load_buffer(buffer.get(), dataSize))
    {
        DV_LOGERROR("Could not parse XML data from " + source.GetName());
        document_->reset();
        return false;
    }

    XmlElement rootElem = GetRoot();
    String inherit = rootElem.GetAttribute("inherit");
    if (!inherit.Empty())
    {
        // The existence of this attribute indicates this is an RFC 5261 patch file
        // If being async loaded, GetResource() is not safe, so use GetTempResource() instead
        XmlFile* inheritedXMLFile = GetAsyncLoadState() == ASYNC_DONE ? DV_RES_CACHE->GetResource<XmlFile>(inherit) :
            DV_RES_CACHE->GetTempResource<XmlFile>(inherit);
        if (!inheritedXMLFile)
        {
            DV_LOGERRORF("Could not find inherited XML file: %s", inherit.c_str());
            return false;
        }

        // Patch this XmlFile and leave the original inherited XmlFile as it is
        unique_ptr<pugi::xml_document> patchDocument = move(document_);
        document_ = make_unique<pugi::xml_document>();
        document_->reset(*inheritedXMLFile->document_);
        Patch(rootElem);

        // Store resource dependencies so we know when to reload/repatch when the inherited resource changes
        DV_RES_CACHE->store_resource_dependency(this, inherit);

        // Approximate patched data size
        dataSize += inheritedXMLFile->GetMemoryUse();
    }

    // Note: this probably does not reflect internal data structure size accurately
    SetMemoryUse(dataSize);
    return true;
}

bool XmlFile::Save(Serializer& dest) const
{
    return Save(dest, "\t");
}

bool XmlFile::Save(Serializer& dest, const String& indentation) const
{
    XMLWriter writer(dest);
    document_->save(writer, indentation.c_str());
    return writer.success_;
}

XmlElement XmlFile::CreateRoot(const String& name)
{
    document_->reset();
    pugi::xml_node root = document_->append_child(name.c_str());
    return XmlElement(this, root.internal_object());
}

XmlElement XmlFile::GetOrCreateRoot(const String& name)
{
    XmlElement root = GetRoot(name);
    if (root.NotNull())
        return root;
    root = GetRoot();
    if (root.NotNull())
        DV_LOGWARNING("XmlFile already has root " + root.GetName() + ", deleting it and creating root " + name);
    return CreateRoot(name);
}

bool XmlFile::FromString(const String& source)
{
    if (source.Empty())
        return false;

    MemoryBuffer buffer(source.c_str(), source.Length());
    return Load(buffer);
}

XmlElement XmlFile::GetRoot(const String& name)
{
    pugi::xml_node root = document_->first_child();
    if (root.empty())
        return XmlElement();

    if (!name.Empty() && name != root.name())
        return XmlElement();
    else
        return XmlElement(this, root.internal_object());
}

String XmlFile::ToString(const String& indentation) const
{
    VectorBuffer dest;
    XMLWriter writer(dest);
    document_->save(writer, indentation.c_str());
    return String((const char*)dest.GetData(), dest.GetSize());
}

void XmlFile::Patch(XmlFile* patchFile)
{
    Patch(patchFile->GetRoot());
}

void XmlFile::Patch(const XmlElement& patchElement)
{
    pugi::xml_node root = pugi::xml_node(patchElement.GetNode());

    for (auto& patch : root)
    {
        pugi::xml_attribute sel = patch.attribute("sel");
        if (sel.empty())
        {
            DV_LOGERROR("XML Patch failed due to node not having a sel attribute.");
            continue;
        }

        // Only select a single node at a time, they can use xpath to select specific ones in multiple otherwise the node set becomes invalid due to changes
        pugi::xpath_node original = document_->select_node(sel.value());
        if (!original)
        {
            DV_LOGERRORF("XML Patch failed with bad select: %s.", sel.value());
            continue;
        }

        if (strcmp(patch.name(), "add") == 0)
            PatchAdd(patch, original);
        else if (strcmp(patch.name(), "replace") == 0)
            PatchReplace(patch, original);
        else if (strcmp(patch.name(), "remove") == 0)
            PatchRemove(original);
        else
            DV_LOGERROR("XMLFiles used for patching should only use 'add', 'replace' or 'remove' elements.");
    }
}

void XmlFile::PatchAdd(const pugi::xml_node& patch, pugi::xpath_node& original) const
{
    // If not a node, log an error
    if (original.attribute())
    {
        DV_LOGERRORF("XML Patch failed calling Add due to not selecting a node, %s attribute was selected.",
            original.attribute().name());
        return;
    }

    // If no type add node, if contains '@' treat as attribute
    pugi::xml_attribute type = patch.attribute("type");
    if (!type || strlen(type.value()) <= 0)
        AddNode(patch, original);
    else if (type.value()[0] == '@')
        AddAttribute(patch, original);
}

void XmlFile::PatchReplace(const pugi::xml_node& patch, pugi::xpath_node& original) const
{
    // If no attribute but node then its a node, otherwise its an attribute or null
    if (!original.attribute() && original.node())
    {
        pugi::xml_node parent = original.node().parent();

        parent.insert_copy_before(patch.first_child(), original.node());
        parent.remove_child(original.node());
    }
    else if (original.attribute())
    {
        original.attribute().set_value(patch.child_value());
    }
}

void XmlFile::PatchRemove(const pugi::xpath_node& original) const
{
    // If no attribute but node then its a node, otherwise its an attribute or null
    if (!original.attribute() && original.node())
    {
        pugi::xml_node parent = original.parent();
        parent.remove_child(original.node());
    }
    else if (original.attribute())
    {
        pugi::xml_node parent = original.parent();
        parent.remove_attribute(original.attribute());
    }
}

void XmlFile::AddNode(const pugi::xml_node& patch, const pugi::xpath_node& original) const
{
    // If pos is null, append or prepend add as a child, otherwise add before or after, the default is to append as a child
    pugi::xml_attribute pos = patch.attribute("pos");
    if (!pos || strlen(pos.value()) <= 0 || strcmp(pos.value(), "append") == 0)
    {
        pugi::xml_node::iterator start = patch.begin();
        pugi::xml_node::iterator end = patch.end();

        // There can not be two consecutive text nodes, so check to see if they need to be combined
        // If they have been we can skip the first node of the nodes to add
        if (CombineText(patch.first_child(), original.node().last_child(), false))
            start++;

        for (; start != end; start++)
            original.node().append_copy(*start);
    }
    else if (strcmp(pos.value(), "prepend") == 0)
    {
        pugi::xml_node::iterator start = patch.begin();
        pugi::xml_node::iterator end = patch.end();

        // There can not be two consecutive text nodes, so check to see if they need to be combined
        // If they have been we can skip the last node of the nodes to add
        if (CombineText(patch.last_child(), original.node().first_child(), true))
            end--;

        pugi::xml_node pos = original.node().first_child();
        for (; start != end; start++)
            original.node().insert_copy_before(*start, pos);
    }
    else if (strcmp(pos.value(), "before") == 0)
    {
        pugi::xml_node::iterator start = patch.begin();
        pugi::xml_node::iterator end = patch.end();

        // There can not be two consecutive text nodes, so check to see if they need to be combined
        // If they have been we can skip the first node of the nodes to add
        if (CombineText(patch.first_child(), original.node().previous_sibling(), false))
            start++;

        // There can not be two consecutive text nodes, so check to see if they need to be combined
        // If they have been we can skip the last node of the nodes to add
        if (CombineText(patch.last_child(), original.node(), true))
            end--;

        for (; start != end; start++)
            original.parent().insert_copy_before(*start, original.node());
    }
    else if (strcmp(pos.value(), "after") == 0)
    {
        pugi::xml_node::iterator start = patch.begin();
        pugi::xml_node::iterator end = patch.end();

        // There can not be two consecutive text nodes, so check to see if they need to be combined
        // If they have been we can skip the first node of the nodes to add
        if (CombineText(patch.first_child(), original.node(), false))
            start++;

        // There can not be two consecutive text nodes, so check to see if they need to be combined
        // If they have been we can skip the last node of the nodes to add
        if (CombineText(patch.last_child(), original.node().next_sibling(), true))
            end--;

        pugi::xml_node pos = original.node();
        for (; start != end; start++)
            pos = original.parent().insert_copy_after(*start, pos);
    }
}

void XmlFile::AddAttribute(const pugi::xml_node& patch, const pugi::xpath_node& original) const
{
    pugi::xml_attribute attribute = patch.attribute("type");

    if (!patch.first_child() && patch.first_child().type() != pugi::node_pcdata)
    {
        DV_LOGERRORF("XML Patch failed calling Add due to attempting to add non text to an attribute for %s.", attribute.value());
        return;
    }

    String name(attribute.value());
    name = name.Substring(1);

    pugi::xml_attribute newAttribute = original.node().append_attribute(name.c_str());
    newAttribute.set_value(patch.child_value());
}

bool XmlFile::CombineText(const pugi::xml_node& patch, const pugi::xml_node& original, bool prepend) const
{
    if (!patch || !original)
        return false;

    if ((patch.type() == pugi::node_pcdata && original.type() == pugi::node_pcdata) ||
        (patch.type() == pugi::node_cdata && original.type() == pugi::node_cdata))
    {
        if (prepend)
            const_cast<pugi::xml_node&>(original).set_value(dviglo::ToString("%s%s", patch.value(), original.value()).c_str());
        else
            const_cast<pugi::xml_node&>(original).set_value(dviglo::ToString("%s%s", original.value(), patch.value()).c_str());

        return true;
    }

    return false;
}

}
