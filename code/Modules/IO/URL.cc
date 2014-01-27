//------------------------------------------------------------------------------
//  URL.cc
//------------------------------------------------------------------------------
#include "Pre.h"
#include "URL.h"
#include "Core/String/StringBuilder.h"
#include "Core/Log.h"
#include "IO/assignRegistry.h"

namespace Oryol {
namespace IO {

using namespace Core;

//------------------------------------------------------------------------------
void
URL::clearIndices() {
    for (int i = 0; i < numIndices; i++) {
        this->indices[i] = InvalidIndex;
    }
}

//------------------------------------------------------------------------------
void
URL::copyIndices(const URL& rhs) {
    for (int i = 0; i < numIndices; i++) {
        this->indices[i] = rhs.indices[i];
    }
}

//------------------------------------------------------------------------------
URL::URL() :
valid(false) {
    this->clearIndices();
}
    
//------------------------------------------------------------------------------
URL::URL(const URL& rhs) :
content(rhs.content),
valid(rhs.valid) {
    this->copyIndices(rhs);
}
    
//------------------------------------------------------------------------------
URL::URL(URL&& rhs) {
    this->content = std::move(rhs.content);
    this->copyIndices(rhs);
    this->valid = rhs.valid;
    rhs.clearIndices();
    rhs.valid = false;
}
    
//------------------------------------------------------------------------------
URL::URL(const char* rhs) :
content(rhs),
valid(false) {
    this->parseIndices();
}
    
//------------------------------------------------------------------------------
URL::URL(const Core::StringAtom& rhs) :
content(rhs),
valid(false) {
    this->parseIndices();
}
    
//------------------------------------------------------------------------------
URL::URL(const Core::String& rhs) :
content(rhs),
valid(false) {
    this->parseIndices();
}
    
//------------------------------------------------------------------------------
void
URL::operator=(const URL& rhs) {
    this->content = rhs.content;
    this->copyIndices(rhs);
    this->valid = rhs.valid;
}
    
//------------------------------------------------------------------------------
void
URL::operator=(URL&& rhs) {
    this->content = rhs.content;
    this->copyIndices(rhs);
    this->valid = rhs.valid;
    rhs.content.Clear();
    rhs.clearIndices();
    rhs.valid = false;
}
    
//------------------------------------------------------------------------------
void
URL::operator=(const char* rhs) {
    this->content = rhs;
    this->parseIndices();
}
    
//------------------------------------------------------------------------------
void
URL::operator=(const Core::StringAtom& rhs) {
    this->content = rhs;
    this->parseIndices();
}
    
//------------------------------------------------------------------------------
void
URL::operator=(const Core::String& rhs) {
    this->content = rhs;
    this->parseIndices();
}
    
//------------------------------------------------------------------------------
bool
URL::operator==(const URL& rhs) {
    return this->content == rhs.content;
}
    
//------------------------------------------------------------------------------
bool
URL::operator!=(const URL& rhs) {
    return this->content != rhs.content;
}
    
//------------------------------------------------------------------------------
const Core::StringAtom&
URL::Get() const {
    return this->content;
}

//------------------------------------------------------------------------------
bool
URL::IsValid() const {
    return this->valid;
}

//------------------------------------------------------------------------------
bool
URL::Empty() const {
    return !this->content.IsValid();
}

//------------------------------------------------------------------------------
bool
URL::parseIndices() {

    this->clearIndices();
    this->valid = false;
    
    if (this->content.IsValid()) {
        StringBuilder builder;
        builder.Set(this->content);
        
        // extract scheme
        this->indices[schemeStart] = 0;
        this->indices[schemeEnd] = builder.FindSubString(0, 8, "://");
        if (EndOfString == this->indices[schemeEnd]) {
            Log::Warn("URL::parseIndices(): '%s' is not a valid URL!\n", this->content.AsCStr());
            this->clearIndices();
            return false;
        }
        
        // extract host fields
        int32 leftStartIndex = this->indices[schemeEnd] + 3;
        int32 leftEndIndex = builder.FindFirstOf(leftStartIndex, EndOfString, "/");
        if (EndOfString == leftEndIndex) {
            leftEndIndex = builder.Length();
        }
        if (leftStartIndex != leftEndIndex) {
            // extract user and password
            int32 userAndPwdEndIndex = builder.FindFirstOf(leftStartIndex, leftEndIndex, "@");
            if (EndOfString != userAndPwdEndIndex) {
                // only user, or user:pwd?
                int32 userEndIndex = builder.FindFirstOf(leftStartIndex, userAndPwdEndIndex, ":");
                if (EndOfString != userEndIndex) {
                    // user and password
                    this->indices[userStart] = leftStartIndex;
                    this->indices[userEnd]   = userEndIndex;
                    this->indices[pwdStart]  = userEndIndex + 1;
                    this->indices[pwdEnd]    = userAndPwdEndIndex;
                }
                else {
                    // only user
                    this->indices[userStart] = leftStartIndex;
                    this->indices[userEnd]   = userAndPwdEndIndex;
                }
                leftStartIndex = userAndPwdEndIndex + 1;
            }
            
            // extract host and port
            int32 hostEndIndex = builder.FindFirstOf(leftStartIndex, leftEndIndex, ":");
            if (EndOfString != hostEndIndex) {
                // host and port
                this->indices[hostStart] = leftStartIndex;
                this->indices[hostEnd]   = hostEndIndex;
                this->indices[portStart] = hostEndIndex + 1;
                this->indices[portEnd]   = leftEndIndex;
            }
            else {
                // only host name, no port
                this->indices[hostStart] = leftStartIndex;
                this->indices[hostEnd]   = leftEndIndex;
            }
        }
        
        // is there any path component?
        if (leftEndIndex != builder.Length()) {
            // extract right-hand-side (path, fragment, query)
            int32 rightStartIndex = leftEndIndex + 1;
            int32 rightEndIndex = builder.Length();
            
            int32 pathStartIndex = rightStartIndex;
            int32 pathEndIndex = builder.FindFirstOf(rightStartIndex, rightEndIndex, "#?");
            if (EndOfString == pathEndIndex) {
                pathEndIndex = rightEndIndex;
            }
            if (pathStartIndex != pathEndIndex) {
                this->indices[pathStart] = pathStartIndex;
                this->indices[pathEnd]   = pathEndIndex;
            }

            // extract query
            if ((pathEndIndex != rightEndIndex) && (builder.At(pathEndIndex) == '?')) {
                int32 queryStartIndex = pathEndIndex + 1;
                int32 queryEndIndex = builder.FindFirstOf(queryStartIndex, rightEndIndex, "#");
                if (EndOfString == queryEndIndex) {
                    queryEndIndex = rightEndIndex;
                }
                if (queryStartIndex != queryEndIndex) {
                    this->indices[queryStart] = queryStartIndex;
                    this->indices[queryEnd] = queryEndIndex;
                }
                pathEndIndex = queryEndIndex;
            }
            
            // extract fragment
            if ((pathEndIndex != rightEndIndex) && (builder.At(pathEndIndex) == '#')) {
                int32 fragStartIndex = pathEndIndex + 1;
                int32 fragEndIndex = builder.FindFirstOf(fragStartIndex, rightEndIndex, "?");
                if (EndOfString == fragEndIndex) {
                    fragEndIndex = rightEndIndex;
                }
                if (fragStartIndex != fragEndIndex) {
                    this->indices[fragStart] = fragStartIndex;
                    this->indices[fragEnd] = fragEndIndex;
                }
            }
        }
        this->valid = true;
    }
    // fallthrough if valid or empty URL
    return true;
}

//------------------------------------------------------------------------------
String
URL::Scheme() const {
    if (InvalidIndex != this->indices[schemeStart]) {
        return String(this->content.AsCStr(), this->indices[schemeStart], this->indices[schemeEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
String
URL::User() const {
    if (InvalidIndex != this->indices[userStart]) {
        return String(this->content.AsCStr(), this->indices[userStart], this->indices[userEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
String
URL::Password() const {
    if (InvalidIndex != this->indices[pwdStart]) {
        return String(this->content.AsCStr(), this->indices[pwdStart], this->indices[pwdEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
String
URL::Host() const {
    if (InvalidIndex != this->indices[hostStart]) {
        return String(this->content.AsCStr(), this->indices[hostStart], this->indices[hostEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
String
URL::Port() const {
    if (InvalidIndex != this->indices[portStart]) {
        return String(this->content.AsCStr(), this->indices[portStart], this->indices[portEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
String
URL::Path() const {
    if (InvalidIndex != this->indices[pathStart]) {
        return String(this->content.AsCStr(), this->indices[pathStart], this->indices[pathEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
String
URL::Fragment() const {
    if (InvalidIndex != this->indices[fragStart]) {
        return String(this->content.AsCStr(), this->indices[fragStart], this->indices[fragEnd]);
    }
    else {
        return String();
    }
}

//------------------------------------------------------------------------------
Map<String, String>
URL::Query() const {
    if (InvalidIndex != this->indices[queryStart]) {
        Map<String, String> query;
        StringBuilder builder;
        builder.Set(this->content.AsCStr(), this->indices[queryStart], this->indices[queryEnd]);
        int32 kvpStartIndex = 0;
        int32 kvpEndIndex = 0;
        do {
            kvpEndIndex = builder.FindFirstOf(kvpStartIndex, EndOfString, "&");
            int32 keyEndIndex = builder.FindFirstOf(kvpStartIndex, kvpEndIndex, "=");
            if (EndOfString != keyEndIndex) {
                // key and value
                String key(builder.GetSubString(kvpStartIndex, keyEndIndex));
                String value(builder.GetSubString(keyEndIndex + 1, kvpEndIndex));
                query.Insert(key, value);
            }
            else {
                // only key
                String key(builder.GetSubString(kvpStartIndex, kvpEndIndex));
                query.Insert(key, String());
            }
            kvpStartIndex = kvpEndIndex + 1;
        }
        while (EndOfString != kvpEndIndex);
        return query;
    }
    else {
        return Map<String, String>();
    }
}

} // namespace IO
} // namespace Oryol