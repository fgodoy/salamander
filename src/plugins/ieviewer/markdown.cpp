// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "cmark-gfm.h"
#include "cmark-gfm-core-extensions.h"
#include "registry.h"

#include "ieviewer.h"
#include "dbg.h"

// TODO: MD viewer doesn't display images, one solution is documented in:
// https://blog.kowalczyk.info/article/g9ne/showing-html-from-memory-in-embedded-web-control-on-windows.html
// https://github.com/sumatrapdfreader/sumatrapdf/blob/master/src/utils/HtmlWindow.cpp (BSD license)

// (Local JS/CSS assets are loaded via SetVirtualHostNameToFolderMapping at http://salamander.local/)

const char* extension_names[] = {
    "autolink",
    "strikethrough",
    "table",
    "tagfilter",
    "tasklist",
    NULL,
};

IStream* ConvertMarkdownToHTML(const char* name)
{
    cmark_gfm_core_extensions_ensure_registered();

    int options = CMARK_OPT_DEFAULT; // Default options
    cmark_parser* parser = cmark_parser_new(options);

    for (const char** it = extension_names; *it; ++it)
    {
        const char* extension_name = *it;
        cmark_syntax_extension* syntax_extension = cmark_find_syntax_extension(extension_name);
        if (!syntax_extension)
        {
            TRACE_E("Invalid syntax extension: " << extension_name);
            cmark_release_plugins();
            return NULL;
        }
        cmark_parser_attach_syntax_extension(parser, syntax_extension);
    }

    FILE* fp = fopen(name, "r");
    if (fp == NULL)
    {
        TRACE_E("fopen failed");
        cmark_release_plugins();
        return NULL;
    }

    size_t bytes;
    char buffer[10000];
    while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        cmark_parser_feed(parser, buffer, bytes);
        if (bytes < sizeof(buffer))
        {
            break;
        }
    }
    fclose(fp);

    cmark_node* doc = cmark_parser_finish(parser);

    char* html = cmark_render_html(doc, options, NULL);

    cmark_node_free(doc);
    cmark_parser_free(parser);

    IStream* oStream = NULL;
    DWORD written;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &oStream);
    if (FAILED(hr))
    {
        TRACE_E("CreateStreamOnHGlobal() failed");
        free(html);
        cmark_release_plugins();
        return NULL;
    }

    char buff[10 * 1024];
    sprintf_s(buff, 
        "<!DOCTYPE html><html lang=\"cs\" dir=\"ltr\"><head><meta charset=\"utf-8\">\n"
        "<link rel=\"stylesheet\" href=\"http://salamander.local/css/githubmd.css\">\n"
        "<script src=\"http://salamander.local/js/mermaid.min.js\"></script>\n"
        "<script>\n"
        "document.addEventListener('DOMContentLoaded', function() {\n"
        "    var blocks = document.querySelectorAll('code.language-mermaid');\n"
        "    blocks.forEach(function(block) {\n"
        "        var pre = block.parentElement;\n"
        "        if (pre && pre.tagName === 'PRE') {\n"
        "            var div = document.createElement('div');\n"
        "            div.className = 'mermaid';\n"
        "            div.textContent = block.textContent;\n"
        "            pre.replaceWith(div);\n"
        "        }\n"
        "    });\n"
        "    mermaid.initialize({ startOnLoad: true, securityLevel: 'loose' });\n"
        "});\n"
        "window.addEventListener('keydown', (e) => {\n"
        "  if (e.key === 'Escape') {\n"
        "    window.chrome.webview.postMessage('Escape');\n"
        "  }\n"
        "  if (e.ctrlKey && (e.key === 'p' || e.key === 'P')) {\n"
        "    e.preventDefault();\n"
        "    window.chrome.webview.postMessage('CtrlP');\n"
        "  }\n"
        "});\n"
        "</script>\n"
        "</head><body><article class=\"markdown-body\">\n");
    oStream->Write(buff, (ULONG)strlen(buff), &written);
    oStream->Write(html, (ULONG)strlen(html), &written);
    sprintf_s(buff, "</article></body></html>\n");
    oStream->Write(buff, (ULONG)strlen(buff), &written);

    // set the pointer to the start of the stream; IE will read from it
    LARGE_INTEGER seek;
    seek.QuadPart = 0;
    oStream->Seek(seek, STREAM_SEEK_SET, NULL);

    free(html);
    cmark_release_plugins();

    return oStream;
}

std::string EscapeHtml(const std::string& text)
{
    std::string result;
    result.reserve((size_t)(text.length() * 1.1));
    for (size_t i = 0; i < text.length(); ++i)
    {
        char c = text[i];
        switch (c)
        {
            case '&':  result.append("&amp;");  break;
            case '<':  result.append("&lt;");   break;
            case '>':  result.append("&gt;");   break;
            case '"':  result.append("&quot;"); break;
            case '\'': result.append("&#039;"); break;
            default:   result.push_back(c);     break;
        }
    }
    return result;
}

const char* GetLanguageClass(const char* filename)
{
    const char* ext = strrchr(filename, '.');
    if (!ext) return "";
    
    if (_stricmp(ext, ".cpp") == 0 || _stricmp(ext, ".hpp") == 0 || _stricmp(ext, ".cc") == 0 || _stricmp(ext, ".h") == 0)
        return "cpp";
    if (_stricmp(ext, ".c") == 0)
        return "c";
    if (_stricmp(ext, ".cs") == 0)
        return "csharp";
    if (_stricmp(ext, ".java") == 0)
        return "java";
    if (_stricmp(ext, ".py") == 0)
        return "python";
    if (_stricmp(ext, ".go") == 0)
        return "go";
    if (_stricmp(ext, ".rs") == 0)
        return "rust";
    if (_stricmp(ext, ".rb") == 0)
        return "ruby";
    if (_stricmp(ext, ".php") == 0)
        return "php";
    if (_stricmp(ext, ".swift") == 0)
        return "swift";
    if (_stricmp(ext, ".kt") == 0 || _stricmp(ext, ".kts") == 0)
        return "kotlin";
    if (_stricmp(ext, ".js") == 0 || _stricmp(ext, ".mjs") == 0 || _stricmp(ext, ".cjs") == 0)
        return "javascript";
    if (_stricmp(ext, ".ts") == 0 || _stricmp(ext, ".tsx") == 0)
        return "typescript";
    if (_stricmp(ext, ".html") == 0 || _stricmp(ext, ".htm") == 0)
        return "xml";
    if (_stricmp(ext, ".css") == 0)
        return "css";
    if (_stricmp(ext, ".sql") == 0)
        return "sql";
    if (_stricmp(ext, ".json") == 0)
        return "json";
    if (_stricmp(ext, ".xml") == 0 || _stricmp(ext, ".xsd") == 0)
        return "xml";
    if (_stricmp(ext, ".yaml") == 0 || _stricmp(ext, ".yml") == 0)
        return "yaml";
    if (_stricmp(ext, ".sh") == 0)
        return "bash";
    if (_stricmp(ext, ".bat") == 0 || _stricmp(ext, ".cmd") == 0)
        return "dos";
    if (_stricmp(ext, ".ps1") == 0)
        return "powershell";
        
    return "";
}

IStream* ConvertCodeToHTML(const char* name)
{
    FILE* fp = fopen(name, "rb");
    if (fp == NULL)
    {
        TRACE_E("fopen failed for code file");
        return NULL;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    std::string rawContent;
    if (size > 0)
    {
        rawContent.resize(size);
        fread(&rawContent[0], 1, size, fp);
    }
    fclose(fp);
    
    std::string escapedCode = EscapeHtml(rawContent);
    const char* langClass = GetLanguageClass(name);
    
    IStream* oStream = NULL;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &oStream);
    if (FAILED(hr))
    {
        TRACE_E("CreateStreamOnHGlobal failed in ConvertCodeToHTML");
        return NULL;
    }
    
    DWORD written;
    char buff[4096];
    
    sprintf_s(buff, 
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">\n"
        "<link rel=\"stylesheet\" href=\"http://salamander.local/css/github-dark.min.css\">\n"
        "<script src=\"http://salamander.local/js/highlight.min.js\"></script>\n"
        "<script src=\"http://salamander.local/js/highlightjs-line-numbers.min.js\"></script>\n"
        "<style>\n"
        "  body {\n"
        "    margin: 0;\n"
        "    padding: 10px;\n"
        "    background-color: #0d1117;\n"
        "    color: #c9d1d9;\n"
        "    font-family: Consolas, Monaco, 'Andale Mono', 'Ubuntu Mono', monospace;\n"
        "    font-size: 14px;\n"
        "  }\n"
        "  pre {\n"
        "    margin: 0;\n"
        "  }\n"
        "  code {\n"
        "    font-family: inherit;\n"
        "  }\n"
        "  .hljs-ln-numbers {\n"
        "    -webkit-user-select: none;\n"
        "    user-select: none;\n"
        "    text-align: right;\n"
        "    color: #8b949e;\n"
        "    border-right: 1px solid #30363d;\n"
        "    padding-right: 10px !important;\n"
        "    min-width: 30px;\n"
        "  }\n"
        "  .hljs-ln-code {\n"
        "    padding-left: 10px !important;\n"
        "  }\n"
        "</style>\n"
        "<script>\n"
        "  document.addEventListener('DOMContentLoaded', (event) => {\n"
        "    hljs.highlightAll();\n"
        "    hljs.initLineNumbersOnLoad();\n"
        "  });\n"
        "  window.addEventListener('keydown', (e) => {\n"
        "    if (e.key === 'Escape') {\n"
        "      window.chrome.webview.postMessage('Escape');\n"
        "    }\n"
        "    if (e.ctrlKey && (e.key === 'p' || e.key === 'P')) {\n"
        "      e.preventDefault();\n"
        "      window.chrome.webview.postMessage('CtrlP');\n"
        "    }\n"
        "  });\n"
        "</script>\n"
        "</head><body><pre><code class=\"%s\">", langClass);
        
    oStream->Write(buff, (ULONG)strlen(buff), &written);
    oStream->Write(escapedCode.c_str(), (ULONG)escapedCode.length(), &written);
    
    const char* footer = "</code></pre></body></html>\n";
    oStream->Write(footer, (ULONG)strlen(footer), &written);
    
    LARGE_INTEGER seek;
    seek.QuadPart = 0;
    oStream->Seek(seek, STREAM_SEEK_SET, NULL);
    
    return oStream;
}
