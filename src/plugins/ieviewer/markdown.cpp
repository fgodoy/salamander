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
