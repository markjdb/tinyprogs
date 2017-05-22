/*-
 * Copyright (c) 2014 Mark Johnston <markj@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer,
 * without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <iostream>
#include <string>

#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>

static std::string progname;

void
usage()
{
    std::cout << "Usage: " << progname << " file1 [file2 [ ... ]]" << std::endl;
    exit(1);
}

int
main(int argc, char **argv)
{
    std::string argv0(argv[0]);
    auto last = argv0.find_last_of("/");
    progname = argv0.substr(last == std::string::npos ? 0 : last + 1);

    if (argc < 2)
        usage();

    const char *tag = argv[1];
    for (int i = 1; argv[i] != NULL; i++) {
        auto *file = new TagLib::MPEG::File(argv[i], false);
        if (file == NULL || !file->isValid()) {
            std::cerr << progname << ": skipping " << argv[i] << std::endl;
            continue;
        }

        const TagLib::ID3v2::FrameListMap &flm(file->ID3v2Tag()->frameListMap());
        for (TagLib::ID3v2::FrameListMap::ConstIterator i = flm.begin(); i != flm.end(); ++i) {
            std::cout << i->first << ": ";
            for (TagLib::ID3v2::FrameList::ConstIterator frame = i->second.begin();
                 frame != i->second.end(); ++frame)
                std::cout << (frame == i->second.begin() ? "" : ", ") << "'" << (*frame)->toString() << "'";
            std::cout << std::endl;
        }

        delete file;
    }

    return (0);
}
