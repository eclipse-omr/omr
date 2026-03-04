<!--
Copyright IBM Corp. and others 2016

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution and
is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following
Secondary Licenses when the conditions for such availability set
forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
General Public License, version 2 with the GNU Classpath
Exception [1] and GNU General Public License, version 2 with the
OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] https://openjdk.org/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
-->

# Contributing to Eclipse OMR

Thanks for your interest in this project.

We welcome and encourage all kinds of contribution to the project, not only code.
This includes bug reports, user experience feedback, assistance in reproducing
issues and more.

If you are new to working with `git`, you may use our [Git Crash Course](doc/GitCrashCourse.md)
to help you get started.

## Issues

This project uses GitHub Issues to track ongoing development, discuss project
plans, and keep track of bugs.  Be sure to search for existing issues before
you create another one.

Visit [our Issues page on GitHub to search and submit](https://github.com/eclipse-omr/omr/issues)

### Labelling

Our GitHub issues get labelled by committers in order to allow easier navigation,
and to flag certain issues as being of interest to certain groups. A PR or Issue
may have multiple labels, as many as needed to provide adequate categorization.

A subset of the labels are documented below.

* [**`good first issue`**](https://github.com/eclipse-omr/omr/labels/good%20first%20issue) generally
  refers to a task that would be suitable for someone new to the project with
  little experience with either the technology or even open-source projects and
  processes. They are intended for anyone who wants to gently get their feet
  wet on building, testing, or familiarizing themselves with part of the code
  base.

* [**`bug`**](https://github.com/eclipse-omr/omr/labels/bug) issues are functional
  problems, errors or unexpected behaviour.

* [**`build/configure`**](https://github.com/eclipse-omr/omr/labels/build%20%2F%20configure)
  labels are used to describe issues with the build and configure system (e.g.,
  makefiles, autotools configure).

* [**`ci`**](https://github.com/eclipse-omr/omr/labels/ci) labels are used for
  issues and enhancements with the continuous integration system for pull
  request testing (e.g., Jenkins, Azure, etc.)

* [**`cmake`**](https://github.com/eclipse-omr/omr/labels/cmake) labels are similar
  to build/configure but they apply specifically to the CMake configuration.

* [**`compiler arch review`**](https://github.com/eclipse-omr/omr/labels/compiler%20arch%20review)
  labels are used to indicate a review of this issue or pull request at the
  [OMR Compiler Architecture meeting](https://github.com/eclipse-omr/omr/issues/2316)
  is requested prior to committing.

* [**`documentation`**](https://github.com/eclipse-omr/omr/labels/documentation)
  labels are used for issues or enhancements to the documentation (either in
  the source code itself or stand-alone documentation files).

* [**`epic`**](https://github.com/eclipse-omr/omr/labels/epic) issues are used to
  group together related issues and to track larger goals in the project across
  issues.

* [**`GSoC Project`**](https://github.com/eclipse-omr/omr/labels/GSoC%20project)
  labels are for potential ideas for Google Summer Of Code projects.

* [**`help wanted`**](https://github.com/eclipse-omr/omr/labels/help%20wanted)
  issues have value to the project but no immediate human resources to
  undertake it. Those that are looking to complete a task that someone isn't
  already working on can consider these.

* [**`license`**](https://github.com/eclipse-omr/omr/labels/license) labels are
  used to annotate issues concerning the source code license.

* [**`meeting`**](https://github.com/eclipse-omr/omr/labels/meeting) labels are
  used to annotate issues pertaining to project meeting agendas or minutes.

* [**`toolchain bug`**](https://github.com/eclipse-omr/omr/labels/toolchain%20bug)
  labels are used to document issues or pull requests that describe or implement
  a workaround to a bug in the development toolchain (such as the compiler) used
  to build OMR.  Toolchain workarounds should be temporary in nature, and the
  intention of this label is to make such workarounds easy to discover in the
  future lest they be lost in the code.

* [**`tooling`**](https://github.com/eclipse-omr/omr/labels/tooling) labels are for
  issues concerning support tooling needed to support any of the code or
  processes within the project.

Labels are also used to classify issues and pull requests by the dominant
Eclipse OMR technology component they affect.  For instance,

| Label | Component | Principal Directories |
| :---- | :---- | :---- |
[**`comp:compiler`**](https://github.com/eclipse-omr/omr/labels/comp%3Acompiler) | Compiler | `compiler`
[**`comp:core`**](https://github.com/eclipse-omr/omr/labels/comp%3Acore) | Core OMR functionality | `include_code`, `omr`
[**`comp:diagnostic`**](https://github.com/eclipse-omr/omr/labels/comp%3Adiagnostic) | Diagnostic services | `ddr`
[**`comp:doc`**](https://github.com/eclipse-omr/omr/labels/comp%3Adoc) | OMR documentation | `doc`
[**`comp:gc`**](https://github.com/eclipse-omr/omr/labels/comp%3Agc) | Garbage collector | `gc`
[**`comp:glue`**](https://github.com/eclipse-omr/omr/labels/comp%3Aglue) | Glue code | `glue`
[**`comp:jitbuilder`**](https://github.com/eclipse-omr/omr/labels/comp%3Ajitbuilder) | JitBuilder | `jitbuilder`
[**`comp:port`**](https://github.com/eclipse-omr/omr/labels/comp%3Aport) | Port library | `port`
[**`comp:test`**](https://github.com/eclipse-omr/omr/labels/comp%3Atest) | Unit tests and testing framework | `fvtest`
[**`comp:thread`**](https://github.com/eclipse-omr/omr/labels/comp%3Athread) | Thread library | `thread`
[**`comp:tril`**](https://github.com/eclipse-omr/omr/labels/comp%3Atril) | Tril infrastructure and tests | `fvtest/tril`
[**`comp:utilities`**](https://github.com/eclipse-omr/omr/labels/comp%3Autil) | OMR utilities | `util`

Further classification by processor architecture, operating system, and bitness
can be achieved with the following labels:

* [**`arch:aarch32`**](https://github.com/eclipse-omr/omr/labels/arch%3Aaarch32)
* [**`arch:aarch64`**](https://github.com/eclipse-omr/omr/labels/arch%3Aaarch64)
* [**`arch:power`**](https://github.com/eclipse-omr/omr/labels/arch%3Apower)
* [**`arch:riscv`**](https://github.com/eclipse-omr/omr/labels/arch%3Ariscv)
* [**`arch:x86`**](https://github.com/eclipse-omr/omr/labels/arch%3Ax86)
* [**`arch:z`**](https://github.com/eclipse-omr/omr/labels/arch%3Az)
<br/>

* [**`os:aix`**](https://github.com/eclipse-omr/omr/labels/os%3Aaix)
* [**`os:linux`**](https://github.com/eclipse-omr/omr/labels/os%3Alinux)
* [**`os:macos`**](https://github.com/eclipse-omr/omr/labels/os%3Amacos)
* [**`os:windows`**](https://github.com/eclipse-omr/omr/labels/os%3Awindows)
* [**`os:zos`**](https://github.com/eclipse-omr/omr/labels/os%3Azos)
<br/>

* [**`bits:32`**](https://github.com/eclipse-omr/omr/labels/bits%3A32)
* [**`bits:64`**](https://github.com/eclipse-omr/omr/labels/bits%3A64)

## Submitting a contribution

You can propose contributions by sending pull requests through GitHub.
Following these guidelines will help us to merge your pull requests smoothly:

1. Eclipse OMR is an Eclipse Foundation project. All contributors must sign the
   [Eclipse Contributor Agreement](https://www.eclipse.org/legal/ECA.php) (or ECA)
   following the instructions on that page before a contribution is accepted.

   See [Eclipse OMR Legal Considerations](#eclipse-omr-legal-considerations) for
   additional ECA considerations when creating project commits as well as copyright
   and license requirements for files.

2. If you're not sure your contribution would be accepted, and want to validate
   your approach or idea before writing code, feel free to open an issue. However,
   not every feature or fix needs an issue. If the problem and fix are cleanly
   connected, and you have the fix in hand, feel free to just submit a pull request.

3. Your pull request is an opportunity to explain both what changes you'd like
   pulled in, but also _why_ you'd like them added. Providing clarity on why
   you want changes makes it easier to accept, and provides valuable context to
   review.

4. Follow the coding style and format of the code you are modifying (see the
   [coding standards](doc/CodingStandard.md)). The code base is yet to be unified
   in style however, so if the file you are editing seems to have a diffferent
   style, defer to the style of the file as you found it.

5. Only use C++ language features supported by our compilers. A list of supported
   features can be found [here](doc/SupportedC++Features.md).

6. Follow the [commit guidelines](#commit-guidelines) described below.

7. Contributions from generative artificial intelligence (GenAI) coding assistants
   are permitted subject to the
   [GenAI Usage Guidelines](#Generative-Artificial-Intelligence-Usage-Guidelines)
   described below.

8. We encourage you to open a pull request early, and mark it as "Work In Progress"
   (prefix the PR title with WIP). This allows feedback to start early, and helps
   create a better end product. Committers will wait until after you've removed
   the WIP prefix to merge your changes.

9. If you are contributing a change to the compiler technology that involves modifications
   to the Testarossa intermediate language (IL) (including, but not limited to,
   adding a new IL opcode, changing the properties of an opcode, or adding a new datatype)
   or, in the opinion of a committer,
   a fundamental element of compiler infrastructure, a committer will request that this
   pull request be presented at an upcoming
   [OMR Compiler Architecture meeting](https://github.com/eclipse-omr/omr/issues/2316)
   to invite community discussion prior to merging.  Issues of the same nature may
   also be asked to be discussed before the same architecture meeting prior to
   producing a pull request.

10. If you are contributing a change to work around a problem with the build
    and test toolchain, you must first open an OMR issue that describes the
    toolchain problem with as much detail as possible, including information
    such as:

    a) A detailed description of the problem (mandatory)

    b) Toolchain versions affected (mandatory)

    c) A reference to an issue in the toolchain's repo tracking the resolution
    of the problem (if known)

    d) Any workarounds (if known)

    e) ETA or expected toolchain release for resolution (if known)

    You must then reference this OMR issue in a code comment implementing the
    compiler workaround. Both the OMR issue and your pull request will be given
    the `toolchain bug` label by a committer.

    The motivation for this requirement is so that problems with toolchains
    affecting the way the code is written have enough information to determine
    when the problem no longer exists and the workaround can be removed at some
    future time.

### Downstream Dependencies

If a downstream project consuming OMR has a dependency on the contents of your pull
request, i.e. the pull request should not be merged until all downstream preparations
have been complete, you as the contributor of the pull request may use the WIP prefix in
addition to comment in the said pull request to inform the committer of your
particular circumstances for merging. That is, you may request a review of the pull
request even though it's marked WIP, informing the committer of your desire to
coordinate when the PR is merged. Once all downstream preparations have been complete
it is your responsibility as the contributor to remove the WIP prefix and inform the
committers that the pull request is ready for merging, pending reviews have been
completed.

## Commit Guidelines

The first line describes the change made. It is written in the imperative mood,
and should say what happens when the patch is applied. Keep it short and
simple. The first line should be less than 70 characters, where reasonable,
and should be written in sentence case preferably not ending in a period.
Leave a blank line between the first line and the message body.

The body should be wrapped at 72 characters, where reasonable.

Include as much information in your commit as possible. You may want to include
designs and rationale, examples and code, or issues and next steps. Prefer
copying resources into the body of the commit over providing external links.
Structure large commit messages with headers, references etc. Remember however
that the commit message is always going to be rendered in plain text.

Please add `[skip ci]` to the commit message when the change doesn't require a
compilation, such as documentation only changes, to avoid unnecessarily wasting
the project's build resources.

Use the commit footer to place commit metadata. The footer is the last block of
contiguous text in the message. It is separated from the body by one or more
blank lines, and as such cannot contain any blank lines. Lines in the footer are
of the form:

```
Key: Value
```

When a commit has related issues or commits, explain the relation in the message
body. You should also leave an `Issue` tag in the footer. However, if this is the
final commit that fixes an issue you should leave a `Closes` tag (or one of the
tags described [here](https://help.github.com/articles/closing-issues-using-keywords/)
in the footer instead which will automatically close the referenced issue when
this pull request is merged.

For example, if this is just one of the commits necessary to address Issue 1234
then the following is a valid commit message:

```
Correct race in frobnicator

This patch eliminates the race condition in issue #1234.

Issue: #1234
```

However, if this is the final commit that addresses the issue then the following
is an acceptable commit message:

```
Correct race in frobnicator

This patch eliminates the race condition in issue #1234.

Closes: #1234
```

### Commit Authorship

Be sure to author commits with the same email address that you used to
sign the Eclipse Contributor Agreement.

If there are multiple authors of a commit they should all be credited
with a `Co-authored-by: Full Name <email address>` line at the footer
of each commit.  Each co-author must sign the ECA.

Each time a commit is pushed, a GitHub CI check will be run
automatically that verifies that the commit author(s) has signed the
Eclipse Contributor Agreement.  If not, the check will fail and the
result will be visible in the pull request.

### Example Commits

Here is an example of a *good* commit:

```
Update and expand the commit guidelines

Elaborate on the style guidelines for commit messages. These new
style guidelines reflect the conversation found in #124.

The guidelines are changed to:
- Provide guidance on how to write a good first line.
- Elaborate on formatting requirements.
- Relax the advice on using issues for nontrivial commits.
- Move issue references from the first line to the message footer.
- Encourage contributors to put more information into the commit
  message.

Issue: #124
```

The first line is meaningful and imperative. The body contains enough
information that the reader understands the why and how of the commit, and its
relation to any issues. The issue is properly tagged.

The following is an example of a *bad* commit:

```
FIX #124: Changing a couple random things in CONTRIBUTING.md.
Also, there are some bug fixes in the thread library.
```

The commit rolls unrelated changes together in a very bad way. There is not
enough information for the commit message to be useful. The first line is not
meaningful or imperative. The message is not formatted correctly and the issue is
improperly referenced.

### Other resources for writing good commits

- http://chris.beams.io/posts/git-commit/

### Install a git commit message hook script to validate local commits

We have a script that attempts to enforce some of these commit guidelines. You
can install it by following instructions similar to the ones below.

```
cd omr_repo_dir
cp scripts/commit-msg .git/hooks
chmod +x .git/hooks/commit-msg
```

If the hook declines your commit, the message will remain in
`omr_repo_dir/.git/COMMIT_EDITMSG`.

Be sure to update your version of the script occasionally as it may evolve as
our commit guidelines evolve.

## Generative Artificial Intelligence Usage Guidelines

The Eclipse OMR project acknowledges that some contributions may be authored in
whole or in part from generative artificial intelligence coding
assistants. Such contributions are permitted per the
[Eclipse Foundation Generative AI Usage Guidelines](https://www.eclipse.org/projects/guidelines/genai/)
which describes contributor and committer responsibilities, copyright and
licensing considerations, and attribution requirements for GenAI assisted
contributions.

All contributors are encouraged to read and understand the advice and
guidelines therein before submitting a contribution to Eclipse OMR even
partially generated by an AI coding assistant.

The Eclipse OMR project is subject to those policies and provides additional
guidelines described below for contributors and committers governing
GenAI-assisted contributions.

### Attribution

Contributions that include GenAI-assisted content must disclose that authorship
in each file where GenAI content is present. Contributors must include a
statement like the following just below the copyright header at the beginning
of the file that acknowledges their AI assistant (assuming such a statement
does not already exist).

```
// Some sections generated by <YOUR AI ASSISTANT>
```

In addition, individual commits that include GenAI authored content must
attribute that at the end of the commit message (before any human authorship
signatures).

```
Co-authored by <YOUR AI ASSISTANT>
```

### Tagging

Some contributors' employers may require their AI-generated code and
documentation contributions to be annotated with inline tags providing
additional metadata about the generated content for legal and license
attribution reasons. Eclipse OMR will accept contributions that include such
metadata with some guidance below for contributors and committers.

#### Tag Usage

Inline tagging of code or documentation generated by coding assistants is not
mandatory.

If tags are used, they must clearly demarcate the boundaries of GenAI-assisted
content (e.g., via paired "begin" and "end" code comments). These boundaries
may span consecutive lines, functions, paragraphs, or entire files. The term
"assisted" is an important qualifier to describe the enclosed content because
it may contain a blend of AI-generated content and human contributions. The
"assisted" distinction is necessary because it may not be possible to clearly
identify just the AI-generated content in the contribution, and it allows for
human changes to the demarcated contribution over time while preserving the
meaning of the tags.

#### Tag Format

At this time, Eclipse OMR does not specify the format of the tags, except that
they must not contain any corporate-specific information or other identifiers
that have no meaning beyond the original contributor (e.g., internal issue
numbers or URLs). Information about the model used to generate the code or
documentation is permitted, however.

Tags must be concise and as minimally-invasive as possible. Tags that are
excessively verbose or impact the readability of the code or documentation may
be rejected by committers at their discretion.

For consistency, tags produced for the same coding assistant must use the same
format regardless of the contributor or pull request.

#### Tag Maintenance

If the contents of a tagged region are modified by a human then any enclosing
tags should remain in place.

If code or documentation within a tagged region is copied in whole or in part
elsewhere in the project, the original surrounding tags should accompany the
copy to its destination. If such code or documentation is copied to a file
that does not already have the Eclipse Foundation GenAI attribution after the
copyright header, such attribution must be added.

Eclipse OMR implements a good-faith policy for contributors and committers to
maintain GenAI tags in the code and documentation for as long as possible.
Existing tags and their boundaries will be maintained on a best-effort basis,
and they should not be removed simply for the sake of removing them.

However, there are legitimate situations where GenAI tagging may be considered
for removal, including if the tags:

* interfere with the readability or understandability of the code or
  documentation,
* become difficult or unwieldy to maintain (e.g., through code refactoring or
  code sharing),
* become obsolete (e.g., information within a tagged section is removed
  entirely),
* become redundant (e.g., an inner set of tags is completely consumed by an
  outer set)

This list is not exhaustive, and the disposition of GenAI tags in a pull
request will ultimately be at the discretion of Eclipse OMR committers who will
balance the spirit of these tagging guidelines with the best interests of the
Eclipse OMR project.

## Eclipse OMR Legal Considerations

### Pull Requests

When a pull request is created (or when new commits are pushed to an existing pull
request), a webservice will automatically run that will verify that each commit
author's email address matches an email address that signed the ECA.
**This service is case-sensitive, so the case of the email addresses must match exactly.**
The success or failure of this webservice will be reported in the GitHub pull request.
Pull requests with a failing ECA check will not be merged.

Eclipse OMR pull requests do not need to be signed, but most contributors do so
as a best practice.

### Copyright Notice and Licensing Requirements

**It is the responsibility of each contributor to obtain legal advice, and
to ensure that their contributions fulfill the legal requirements of their
organization. This document is not legal advice.**

OMR is dual-licensed under the Eclipse Public License 2.0 and the Apache
License v2.0. Any previously unlicensed contribution should be released under
the same license.

If you wish to contribute code under a different license, you must consult with
a committer before contributing. For any scenario not covered by this document,
please discuss the copyright notice and licensing requirements with a committer
before contributing.

The copyright date reported in the copyright template to appear in each file is
the 4-digit year in which the file was first authored. Note that this may be
different from the year it is actually merged into the repository. If
subsequent contributions are made to an existing file the copyright date must
not be changed.

The template for the copyright notice and dual-license is as follows:

```c
/*******************************************************************************
 *  Copyright ${author} and others 2017
 *
 *  This program and the accompanying materials are made available under
 *  the terms of the Eclipse Public License 2.0 which accompanies this
 *  distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 *  or the Apache License, Version 2.0 which accompanies this distribution and
 *  is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 *  This Source Code may also be made available under the following
 *  Secondary Licenses when the conditions for such availability set
 *  forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 *  General Public License, version 2 with the GNU Classpath
 *  Exception [1] and GNU General Public License, version 2 with the
 *  OpenJDK Assembly Exception [2].
 *
 *  [1] https://www.gnu.org/software/classpath/license.html
 *  [2] https://openjdk.org/legal/assembly-exception.html
 *
 *  SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/
```

Dummy
