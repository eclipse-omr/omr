<!--
Copyright IBM Corp. and others 2026

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution
and is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following Secondary
Licenses when the conditions for such availability set forth in the
Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
version 2 with the GNU Classpath Exception [1] and GNU General Public
License, version 2 with the OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] https://openjdk.org/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
-->

# Const Ref Privatization

## Acyclic Analysis Overview

In example CFGs, control flow is from top to bottom (acyclic). Blocks marked with * are all loading some particular const ref, and blocks marked with + are all loading some *other* particular const ref.

### First Analysis Pass

So the first pass identifies candidates where there is a redundancy that could potentially be eliminated. And if there aren't any, we're done.

Here's an example where a const ref is loaded in multiple blocks, but there's plainly no redundancy:

       A
      / \
    *B   C*
      \ /
       D

### Second Analysis Pass

The second pass identifies points where a candidate is loaded for the first time and has at least some minimal chance of getting reused later (currently 5%). These are natural places to initialize the temp. Again, if there aren't any, we're done.

In order to make that determination, it computes the probability that each candidate will be loaded after control leaves the current CFG node, and also the probability that it will be loaded in the current CFG node itself (as a block) or later.

This rules out cases where there is redundancy, but it isn't frequent enough to bother with, even if it occurs at a point that is not cold enough to completely ignore/penalize. For example: 

      A*
     / \
    B   C* (freq 6)
     \ /
      D

### Third Analysis Pass

Most of the third pass is deciding where else it might be beneficial to initialize temps. There is a lot of flexibility in not being limited to points where a candidate is loaded for the first time.

#### Initialize in Ignored Predecessors

Suppose `_` is ignored, e.g. an actually-cold cold call:

       A
      / \
    *B   _ (cold)
      \ /
      *D

Initializing just at B wouldn't be enough to guarantee that the temp is initialized by the time we reach D. In this case, the third pass will decide to also initialize it at `_`. (Really, it decides to initialize it along edges incoming to D, but in this situation the actual initialization will always be at `_` because it has only one successor, and because as an ignored block the analysis considers it OK to penalize.)

#### Initialize in Regular Predecessors

As another example:

       A
      / \
    *B   C+
      \ /
      *D+

Now * and + are individually in the same situation as * in the previous example, but with respect to different predecessors. In this case, the third pass will decide to also initialize + at B and * at C. (Again, it actually decides to initialize along incoming edges, but the predecessors each have only one successor.)

(Note: As currently implemented, the loads in D itself will not cause the initialization to be placed in B and C as described, because that part of the analysis is looking at D's *future* load probabilities instead of the load probabilities that include D itself. A trivial modification to the CFG would make the example work with the current implementation: split D into two blocks, with the const ref loads in the second block. Regardless, this appears to be a simple oversight, and there is a TODO comment suggesting a fix.)

#### Initialize Along a Critical Edge

If there's a critical edge, then the third pass can decide to split it to create a place to initialize the temp, e.g.

       A
      / \
    *B   C+
      \ / \
      *D+  ...

Now we don't want to initialize * at C just for the benefit of D, since C might go elsewhere.  In this case, the third pass will decide to split the critical edge and initialize * in the new block. (A potential point of improvement for the future is that maybe all of the other successors of C are ignored, in which case it still would have been fine to use C directly.)

The analysis will refuse to commit to creating more than one new block per original merge point for the purpose of splitting critical edges. 

#### Initialize at Existing Const Ref Load

Finally, maybe there is an existing load of the candidate in the current block. If the temp is not necessarily initialized based on the earlier heuristics, and if it's highly likely (currently >=95%) to be loaded again in the future, then the third pass will decide to initialize it in the current block. For example:

          A
         / \
       *B   C+
       / \ / \
    ...  *D+  ...
         / \
       *E   ...

In this case, because we'd need to initialize different sets of temps along BD and CD, and because we refuse to split both edges separately, we can't guarantee that either temp would be initialized on entry to D. But if E is a likely enough successor, the third pass will still decide that it's worth initializing * at D.

#### Determine Definite Initialization Status

While the third pass is deciding where else to initialize temps, it's determining for each CFG node which temps are definitely initialized (by the time control leaves the node). It uses that info to avoid requesting initialization where it's definitely already been done.

#### Seed Liveness

The last task of the third pass is to seed the liveness with essentially the gen set. We can reuse the value from an earlier load if (a) there is a load in the current block, and (b) the value is potentially available, and (c) we can guarantee that the temp for the candidate will be initialized in time. Essentially, this is a decision that the const ref load in the current block will be replaced with a temp load (`performTransformation()` permitting), and so the temp will be live.

(Note: There is a TODO to additionally require that (d) the temp is not supposed to be initialized in the current block itself.)

If no candidate was found to be live anywhere, then we don't intend to introduce any temp loads, and so temp stores are also unnecessary, and we're done. Here's an example of how that could happen:

        A
       / \
      B   C*
       \ /
        D
       / \
    ...   E*

Here, the first pass would find that E might be able to use the potentially available value (from C). Then suppose that there's about a 50% chance that D will go to E. The second pass will see that the value from C has a nontrivial chance of getting reused, and so it will decide to initialize the temp at C. In the third pass, * is not guaranteed to be initialized on entry to D, and D won't be selected for initialization because the future load probability isn't high enough to justify it. This means that * is also not guaranteed to be initialized on entry to E, so E can't rely on being able to do a temp load, and therefore * is not considered to be live at E.

### Fourth Analysis Pass

The fourth pass propagates liveness backwards so that it's possible to skip initializing temps at points where they would just end up being dead anyway. This can (perhaps surprisingly) help candidates that have been selected for transformation. In particular, a candidate might be used in unrelated parts of the CFG, where it helps to transform it in one place but not another. To see that, let's expand the example from the initial seeding of liveness with the gen set (in the third pass):

                  A
                 / \
      +---------B   C*
      |          \ /
     *F           D
     /|          / \
    G |       ...   E*
     \|
     *H

Here * will not be considered live at E, just like in the original version of the example. However, it now *will* be considered live at H, and so it will undergo transformation. Propagating the liveness backward will allow the transformation to tell that initialization is required at F but not at C.
