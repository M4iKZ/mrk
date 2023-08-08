
#include "parser.hpp"

ParsedURL parseURL(const std::string& input)
{
    std::string buffer;
    ParsedURL parsedURL;
    ParseURLState state = ParseURLState::SCHEMA;
    for (char c : input)
    {
        switch (state)
        {
        case ParseURLState::SCHEMA:
            if (c == ':' && (buffer == "http" || buffer == "https"))
            {
                parsedURL.schema = buffer;
                buffer.clear();
                state = ParseURLState::HOST;
            }
            else if (c == ':')
            {
                parsedURL.schema = "http"; // Default to http
                parsedURL.host = buffer; // Captured host before
                buffer.clear();
                state = ParseURLState::PORT;
            }
            else if (c == '/')
            {
                parsedURL.schema = "http";  // Default to http 
                parsedURL.host = buffer; // Captured host before
                buffer.clear();
                state = ParseURLState::URI;

                parsedURL.uri += "/";
            }
            else
                buffer += c;

            break;

        case ParseURLState::HOST:
            if (c == ':')
            {
                parsedURL.host = buffer;
                buffer.clear();
                state = ParseURLState::PORT;
            }
            else if (c == '/')
            {
                if (buffer.empty() || buffer == "/")
                {
                    buffer.clear(); // HTTP or HTTPS //
                    continue;
                }

                parsedURL.host = buffer;
                buffer.clear();
                state = ParseURLState::URI;

                parsedURL.uri += "/";
            }
            else
                buffer += c;

            break;

        case ParseURLState::PORT:
            if (c == '/')
            {
                parsedURL.port = buffer.empty() ? (parsedURL.schema == "https" ? "443" : "80") : buffer;
                buffer.clear();
                state = ParseURLState::URI;

                parsedURL.uri += "/";
            }
            else
                buffer += c;

            break;

        case ParseURLState::URI:
            parsedURL.uri += c;
            break;
        }
    }

    if (state == ParseURLState::SCHEMA)
    {
        parsedURL.schema = "http";  // Default to http if no schema provided
        parsedURL.host = buffer;    // Treat the buffer as part of the host
    }
    else if (state == ParseURLState::HOST)
    {
        parsedURL.host = buffer;
    }
    if (state == ParseURLState::PORT)
    {
        parsedURL.port = buffer.empty() ? (parsedURL.schema == "https" ? "443" : "80") : buffer;
    }

    if (parsedURL.port.empty())
        parsedURL.port = parsedURL.schema == "https" ? "443" : "80";

    if (parsedURL.uri.empty())
        parsedURL.uri = "/";

    return parsedURL;
}

int extractStatusCode(const std::vector<char>& response) 
{
    ParseState state = ParseState::START;
    std::string statusCodeStr;

    for (char c : response) 
    {
        switch (state) 
        {
        case ParseState::START:
            if (c == 'H') 
                state = ParseState::HTTP;            
            break;

        case ParseState::HTTP:
            if (c == ' ') 
                state = ParseState::STATUS_CODE;            
            break;

        case ParseState::STATUS_CODE:
            if (c == ' ') 
                state = ParseState::FINISH;
            
            else 
                statusCodeStr += c;
            break;

        case ParseState::FINISH:            
            break;
        }
    }

    try 
    {
        return std::stoi(statusCodeStr);
    }
    catch (const std::exception& e) {}

    return -1; // Invalid status code
}

bool getContentLength(const std::vector<char>& data, size_t& headersize)
{
    std::string response(data.begin(), data.end());

    if(!headersize)
        headersize = response.find("\r\n\r\n") + 4; // Add Double line break size

    size_t pos = response.find("Content-Length: ");
    if (pos != std::string::npos)
    {
        // Extract the content length value
        size_t valueStart = pos + strlen("Content-Length: ");
        size_t valueEnd = response.find_first_of("\r\n", valueStart);
        std::string contentlength = response.substr(valueStart, valueEnd - valueStart);

        if (data.size() - headersize < std::stoi(contentlength))
            return false;
    }

    return true;
}