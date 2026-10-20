
#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <cctype>

using namespace std;

// Utility class for HTML escaping
class HTMLUtils {
public:
    static string escapeHTML(const string& text) {
        string result;

        for (char ch : text) {
            if (ch == '&') result += "&amp;";
            else if (ch == '<') result += "&lt;";
            else if (ch == '>') result += "&gt;";
            else if (ch == '"') result += "&quot;";
            else if (ch == '\'') result += "&#39;";
            else result += ch;
        }

        return result;
    }
};

// Abstract base class
class Parser {
public:
    virtual string parse(const string& text) = 0;
    virtual ~Parser() {}
};

// Derived class for inline Markdown
class InlineParser : public Parser {
private:
    bool validURL(const string& url) {
        if (url.empty()) return false;

        for (char ch : url) {
            if (isspace(static_cast<unsigned char>(ch)) ||
                ch == '"' || ch == '\'' || ch == '<' ||
                ch == '>' || ch == '\\') {
                return false;
            }
        }

        if (url.find(':') != string::npos) {
            return url.rfind("https://", 0) == 0 ||
                   url.rfind("http://", 0) == 0 ||
                   url.rfind("mailto:", 0) == 0;
        }

        return true;
    }

public:
    string parse(const string& text) override {
        string result;
        size_t i = 0;

        while (i < text.size()) {
            if (text.substr(i, 2) == "**") {
                size_t end = text.find("**", i + 2);

                if (end != string::npos) {
                    result += "<strong>" +
                        parse(text.substr(i + 2, end - i - 2)) +
                        "</strong>";
                    i = end + 2;
                    continue;
                }
            }

            if (text[i] == '*') {
                size_t end = text.find('*', i + 1);

                if (end != string::npos) {
                    result += "<em>" +
                        parse(text.substr(i + 1, end - i - 1)) +
                        "</em>";
                    i = end + 1;
                    continue;
                }
            }

            if (text[i] == '`') {
                size_t end = text.find('`', i + 1);

                if (end != string::npos) {
                    result += "<code>" +
                        HTMLUtils::escapeHTML(
                            text.substr(i + 1, end - i - 1)) +
                        "</code>";
                    i = end + 1;
                    continue;
                }
            }

            if (text[i] == '[') {
                size_t labelEnd = text.find(']', i + 1);

                if (labelEnd != string::npos &&
                    labelEnd + 1 < text.size() &&
                    text[labelEnd + 1] == '(') {
                    size_t urlEnd = text.find(')', labelEnd + 2);

                    if (urlEnd != string::npos) {
                        string label =
                            text.substr(i + 1, labelEnd - i - 1);
                        string url =
                            text.substr(labelEnd + 2,
                                        urlEnd - labelEnd - 2);

                        if (validURL(url)) {
                            result += "<a href=\"" +
                                HTMLUtils::escapeHTML(url) + "\">" +
                                parse(label) + "</a>";
                        } else {
                            result += HTMLUtils::escapeHTML(
                                text.substr(i, urlEnd - i + 1));
                        }

                        i = urlEnd + 1;
                        continue;
                    }
                }
            }

            result += HTMLUtils::escapeHTML(string(1, text[i]));
            ++i;
        }

        return result;
    }
};

// Derived class for block-level Markdown
class BlockParser : public Parser {
private:
    InlineParser inlineParser;
    bool inList, inCode, inQuote, inParagraph;
    string paragraph, html, codeText;

    bool startsWith(const string& text, const string& prefix) {
        return text.rfind(prefix, 0) == 0;
    }

    bool isListItem(const string& line) {
        return startsWith(line, "- ") || startsWith(line, "* ");
    }

    bool isHeading(const string& line) {
        return !line.empty() && line[0] == '#';
    }

    bool isHorizontalRule(const string& line) {
        return line == "---" || line == "***" || line == "___";
    }

    void closeParagraph() {
        if (inParagraph) {
            html += "<p>" + inlineParser.parse(paragraph) + "</p>\n";
            paragraph.clear();
            inParagraph = false;
        }
    }

    void closeList() {
        if (inList) {
            html += "</ul>\n";
            inList = false;
        }
    }

    void closeQuote() {
        if (inQuote) {
            html += "</blockquote>\n";
            inQuote = false;
        }
    }

    void closeBlocks() {
        closeParagraph();
        closeList();
        closeQuote();
    }

    void parseHeading(const string& line) {
        closeBlocks();

        size_t count = 0;
        while (count < line.size() && line[count] == '#')
            count++;

        if (count <= 6 && count < line.size() && line[count] == ' ') {
            string tag = to_string(count);
            string content = line.substr(count + 1);

            html += "<h" + tag + ">" +
                inlineParser.parse(content) +
                "</h" + tag + ">\n";
        } else {
            paragraph = line;
            inParagraph = true;
        }
    }

    void parseListItem(const string& line) {
        closeParagraph();
        closeQuote();

        if (!inList) {
            html += "<ul>\n";
            inList = true;
        }

        html += "<li>" +
            inlineParser.parse(line.substr(2)) +
            "</li>\n";
    }

    void parseQuote(const string& line) {
        closeParagraph();
        closeList();

        if (!inQuote) {
            html += "<blockquote>\n";
            inQuote = true;
        }

        string content = line.substr(1);

        if (!content.empty() && content[0] == ' ')
            content.erase(0, 1);

        html += "<p>" + inlineParser.parse(content) + "</p>\n";
    }

public:
    BlockParser()
        : inList(false), inCode(false),
          inQuote(false), inParagraph(false) {}

    string parse(const string& text) override {
        html.clear();
        paragraph.clear();
        codeText.clear();
        inList = inCode = inQuote = inParagraph = false;

        istringstream input(text);
        string line;

        while (getline(input, line)) {
            if (startsWith(line, "```")) {
                if (!inCode) {
                    closeBlocks();
                    inCode = true;
                    codeText.clear();
                } else {
                    html += "<pre><code>" +
                        HTMLUtils::escapeHTML(codeText) +
                        "</code></pre>\n";
                    codeText.clear();
                    inCode = false;
                }
                continue;
            }

            if (inCode) {
                codeText += line + "\n";
                continue;
            }

            if (line.empty()) {
                closeBlocks();
                continue;
            }

            if (isHeading(line)) {
                parseHeading(line);
            } else if (isListItem(line)) {
                parseListItem(line);
            } else if (line[0] == '>') {
                parseQuote(line);
            } else if (isHorizontalRule(line)) {
                closeBlocks();
                html += "<hr>\n";
            } else {
                closeList();
                closeQuote();

                if (!inParagraph) {
                    paragraph = line;
                    inParagraph = true;
                } else {
                    paragraph += "\n" + line;
                }
            }
        }

        if (inCode) {
            html += "<pre><code>" +
                HTMLUtils::escapeHTML(codeText) +
                "</code></pre>\n";
        }

        closeBlocks();
        return html;
    }
};

// Main converter class
class MarkdownConverter {
private:
    unique_ptr<Parser> blockParser;

public:
    MarkdownConverter() {
        blockParser = make_unique<BlockParser>();
    }

    string convert(const string& markdown) {
        return blockParser->parse(markdown);
    }
};

int main() {
    ostringstream input;
    input << cin.rdbuf();

    MarkdownConverter converter;
    cout << converter.convert(input.str());

    return 0;
}
