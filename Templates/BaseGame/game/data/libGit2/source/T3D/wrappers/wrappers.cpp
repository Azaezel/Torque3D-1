//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "git2.h"

#include "console/engineAPI.h"

DefineEngineFunction(git_init, String, (), ,
        "@brief initialize libGit2.\n\n")
{
	int error = git_libgit2_init();
	if (error < 0) {
		const git_error *e = git_error_last();
		return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
	}
	return "";
}

DefineEngineFunction(git_shutdown, String, (), ,
        "@brief Logs a message to the console.\n\n"
        "@param message The message text.\n"
        "@note By default, messages will appear white in the console.\n"
        "@ingroup Logging")
{
	int error = git_libgit2_shutdown();
	if (error < 0) {
		const git_error *e = git_error_last();
		return String::ToString("Error %d/%d: %s\n", error, e->klass, e->message);
	}
	return "";
}
