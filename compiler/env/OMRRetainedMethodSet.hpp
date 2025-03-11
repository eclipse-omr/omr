/*******************************************************************************
 * Copyright IBM Corp. and others 2024
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#ifndef OMR_RETAINEDMETHODSET_INCL
#define OMR_RETAINEDMETHODSET_INCL

#include <stddef.h>
#include "env/jittypes.h"
#include "infra/map.hpp"

namespace TR {
class Compilation;
}
class TR_ResolvedMethod;

namespace OMR {

/**
 * \brief A RetainedMethodSet represents the set of methods that are guaranteed
 * to remain live under some assumptions.
 *
 * The main such assumption is that some particular method is live.
 *
 * In a project that allows methods to be unloaded, this is useful to prevent a
 * JIT body from outliving any code that has been inlined into it. For more on
 * the motivation for such prevention, see the "Motivation" section below.
 *
 * The set is not necessarily complete. There may be methods that will remain
 * loaded that cannot be or simply have not been identified as such.
 *
 * Each instance can have a parent and thereby belong to a tree. The parent
 * represents the set of methods that are guaranteed to remain loaded in some
 * surrounding context. This facility is used during inlining to track for each
 * call path the set of methods that will be guaranteed to remain loaded on the
 * assumption that that call path will be inlined.
 *
 * Often, the method named at a call site and the chosen call target will be
 * guaranteed to remain loaded, in which case the RetainedMethodSet can be
 * shared between the caller and the callee. But sometimes it's desireable to
 * inline a method that could (as far as we know) be unloaded before the
 * outermost method, and it may be justifiable in order to do so to arrange
 * somehow for it to outlive the compiled code that will result from the
 * current compilation. In such a case, we create a child RetainedMethodSet
 * that represents the set of methods that will remain loaded at least as long
 * as the JIT body, assuming that such an arrangement is made. When the method
 * is actually inlined, it's necessary to commit to put that arrangement into
 * effect.
 *
 * This class does not know how to determine the relationships between methods
 * in a given project. It is designed to be subclassed in any project that
 * allows unloading. The default implementation assumes that all methods are
 * guaranteed to remain live/loaded, which is suitable in projects that don't
 * support unloading, or when unloading is disabled at runtime, or for
 * debugging if unloading can be assumed not to occur.
 *
 * \b Motivation
 *
 * Why prevent the JIT body from outliving methods inlined into it? First code
 * motion, and then the more proximate motivation of constant references.
 *
 * It has historically been possible for a JIT body to continue to run even
 * after one of its inlined methods has been unloaded. A guard would be patched
 * to prevent execution from entering the inlined body. However, this strategy
 * of patching guards is by itself insufficient to allow for code motion.
 *
 * Code motion can cause an expression to be evaluated speculatively. The
 * speculative location might no longer be protected by the expected guard, and
 * the expression might not be safe to evaluate after unloading, e.g. it might
 * use instanceof to check that an object is an instance of a type that no
 * longer exists, or it might access a static field that no longer exists.
 *
 * Loop versioner has attempted to avoid the instanceof situation by checking
 * isUnloadAssumptionRequired() in a way that's both unnecessarily conservative
 * and most likely incomplete. It's easy to accidentally assume that as the JIT
 * body continues to run, the inlined methods (or rather, whatever they refer
 * to) will remain loaded. To prevent such assumptions from becoming bugs, we
 * can make them hold.
 *
 * Additionally, using the information represented here, it may be possible to
 * provide a more precise implementation of isUnloadAssumptionRequired() for
 * cases where it will still be necessary, e.g. for profiled checkcast types.
 *
 * Now on to the topic of constant references. Historically "constant folding"
 * of references has been done by leaving the original load in place but
 * replacing its symref with an "improved" version that has a known object
 * index. That is, the folding itself has been treated as an analysis result
 * recorded in the IL - much like a node flag - rather than as a transformation
 * that puts the IL into a form in which the constant value is naturally
 * guaranteed, as we would for a primitive constant (e.g. iconst). As a result,
 * it has been correct to "fold" a reference only if it's possible to guarantee
 * that repeating the original operation (at its point of occurrence) will
 * always produce the same result. However, there are cases where it is
 * desirable to allow folding despite the possibility of a later mutation. For
 * this reason (and another performance reason), constant references will allow
 * the compiler to treat references in nearly the same way as primitives when
 * they are constant. Instead of adding a known object index as an analysis
 * result, we'll transform the node so that it always produces the expected
 * reference. This will be achieved by creating a fresh immutable reference and
 * loading from that instead of doing the original operation. See this OpenJ9
 * issue for more detail:
 *
 *     https://github.com/eclipse-openj9/openj9/issues/16616
 *
 * Please excuse the prevalence of Java in the examples below. It's the most
 * expedient way to illustrate, but the concepts should be relatively general.
 *
 * With that context, consider the lifetime of a constant reference. It exists
 * for the benefit of a particular JIT body, and as long as that JIT body might
 * run, it might make use of the reference, so the reference must still exist.
 * For simplicity, no attempt will be made to track "which part" of the JIT
 * body needs the constant. And clearly once the JIT body can't run anymore,
 * the constant reference will be unused, and there will be no point in its
 * continued existence. So the constant reference should have exactly the same
 * lifetime as the JIT body - or really, the bulk of the JIT body, ignoring the
 * stub left behind by invalidation. And indeed the reference will be embedded
 * in the JIT body. We can think of it as though the JIT body were an object
 * and the constant reference were one of its fields. In particular, as long as
 * the JIT body exists and is valid, the constant reference will keep its
 * referent from being collected.
 *
 * For the sake of example, suppose that the unit of unloading is the class,
 * and consider these Java classes:
 *
 *    public abstract class AbstractFoo {
 *        public abstract void foo();
 *    }
 *
 *    public class Bar {
 *        public static void callFoo(AbstractFoo f) { f.foo(); }
 *    }
 *
 *    public class Foo extends AbstractFoo {
 *        private static final Foo TOTAL = new Foo();
 *        private int n;
 *        public void foo() { TOTAL.n++; n++; }
 *        ...
 *    }
 *
 * The program could dynamically load Foo, pass instances of it to callFoo()
 * for a while, and then eventually stop using Foo altogether and allow it to
 * be unloaded.
 *
 * If callFoo() is compiled and Foo.foo() is inlined into it (maybe based on
 * profiling, or maybe because it's a single implementer), and if TOTAL is
 * folded, and crucially if the JIT body of callFoo() is allowed to outlive
 * Foo, then it will live as long as Bar, and there will essentially be a
 * a hidden reference from Bar to Foo.TOTAL, which will prevent Foo from being
 * unloaded. This example memory leak is very small, but there is no limit to
 * the size of the object graph that could be leaked in this way.
 *
 * This is where RetainedMethodSet comes in. It will either prevent Foo.foo()
 * from getting inlined (dontInlineUnloadableMethods), or with inlining it will
 * take note that the JIT body can't be allowed to outlive Foo (bond). With
 * cooperation from the downstream compiler, a runtime assumption will be
 * created to invalidate the JIT body when Foo is unloaded. With cooperation
 * from the GC, the constant reference belonging to callFoo() will be scanned
 * only when Foo is still live. Once the GC fails to find that Foo is live, it
 * will ignore this constant reference, so it won't prevent unloading, and Foo
 * will be unloaded as expected. The unloading will invalidate the JIT body so
 * that the ignored reference won't be subsequently used.
 *
 * Why is it ever okay to keep a hidden reference to an object if the original
 * reference might change? i.e. why is inlining singled out? There are three
 * reasons:
 *
 * 1. Without the problematic sort of inlining situation described above, there
 *    can be a leak only if the fields considered constant by the compiler
 *    are actually mutated later on. With the inlining, as can be seen from the
 *    example, it would be possible to leak memory without such mutation.
 *
 * 2. Keeping a hidden reference to the object is necessary in order to allow
 *    constant folding in the first place. If we can't do so, then we have no
 *    business folding constants at all.
 *
 * 3. With JIT bodies prevented from outliving their inlined methods, it will
 *    be possible to explain why a particular object is still referenced purely
 *    in terms of constant folding, e.g. Foo.foo() referred to this object via
 *    Foo.TOTAL, which was constant folded, and Foo.foo() still exists, so this
 *    object can still be referenced. We avoid unnecessarily allowing inlining
 *    to factor into this kind of justification. Though of course inlining will
 *    still affect which expressions the compiler sees, and therefore which it
 *    bothers to fold, and an effort will be made not to keep unnecessary
 *    references, it's extraneous in terms of justifying why the compiler
 *    should be \e allowed to keep a reference to an object.
 *
 * \e Keepalive. The keepalive facility is to be used in cases where a call
 * site has been refined based on a known object that reflectively identifies a
 * method to be called. This is essentially another kind of constant folding,
 * but where the constant is a method instead of an IL value. Since we would be
 * justified in keeping a reference to the reflective object, which would
 * prevent the method it represents from being unloaded (while the JIT body is
 * still live), we should be equally justified in arranging to keep a reference
 * specifically in order to prevent the method from being unloaded. This is
 * useful in cases where later mutation is possible but can be correctly
 * ignored, e.g. when customizing a MethodHandle in Java. Without a keepalive,
 * later mutation could cause the original method to become unreachable and to
 * be unloaded, unnecessarily invalidating the JIT body into which it was
 * inlined.
 *
 * Keepalives are needed separately from the original known objects because
 * constant references will only be created for known objects that might be
 * used at runtime, i.e. those that are still loaded somewhere in the IL at
 * code generation time.
 *
 * In all analysis, bonds (which restrict the lifetime of the JIT body) take
 * precedence over keepalives (which extend the lifetime of inlined methods).
 * To see why, consider another Java example:
 *
 *     public abstract class AbstractBase {
 *         public abstract void callee();
 *     }
 *
 *     public class Caller {
 *         public static void callTwice(AbstractBase b1, AbstractBase b2) {
 *             b1.callee();
 *             b2.callee();
 *         }
 *     }
 *
 *     public class Derived1 extends AbstractBase {
 *         private static final MethodHandle DERIVED2_FOO = ...; // Derived2.foo()
 *         public void callee() { DERIVED2_FOO.invokeExact(); }
 *         public static void foo() { ... }
 *     }
 *
 *     public class Derived2 extends AbstractBase {
 *         private static final MethodHandle DERIVED1_FOO = ...; // Derived1.foo()
 *         public void callee() { DERIVED1_FOO.invokeExact(); }
 *         public static void foo() { ... }
 *     }
 *
 * The program could dynamically load Derived1 and Derived2 (separately, so
 * that it's only the MethodHandles tying their lifetimes together), spend a
 * while passing instances of them as b1 and b2, resp., to callTwice(), and
 * then eventually stop using them and allow them to be unloaded.
 *
 * Suppose that callTwice() is compiled, and that Derived1.callee() and
 * Derived2.callee() are inlined at the call sites for b1 and b2, resp.
 * If there is no further inlining, then they will be inlined with bonds, and
 * unloading will work as normal. If OTOH the foo() calls were also inlined,
 * they would be inlined with keepalives. If keepalives took precedence over
 * bonds, then these keepalives would cause callTwice() to prevent unloading of
 * both Derived1 and Derived2, even though they're supposed to be unloadable,
 * and even though they would have been unloadable with less inlining.
 *
 * Because of the possibility of this kind of situation, all keepalives and
 * bonds are collected, but whenever a bond and a keepalive both apply to the
 * same method, the bond is used and the keepalive is ignored. Essentially, if
 * there is one call path that handles that method using bond, then a bond will
 * be used even if other call paths handle the same method using keepalive.
 *
 * Keepalives are intended to be implemented as additional constant references.
 * To see why this scheme successfully prevents the JIT body from outliving its
 * inlined methods, note that every inlined method is covered by one of the
 * following cases:
 *
 * 1. Analysis found that even without any intervention it's guaranteed to live
 *    at least as long as the outermost method, or
 *
 * 2. A bond will be used, or
 *
 * 3. A keepalive will be used.
 *
 * If the JIT body is live, then all inlined methods in case 1 and 2 are live
 * as well. Those in case 1 simply can't be unloaded before the outermost
 * method, and if one in case 2 were unloaded, then the JIT body would have
 * been invalidated. Since the outermost method and all of the bonded methods
 * are still loaded, the constant references are live, including all of the
 * keepalive references for inlined methods in case 3, so those must still be
 * loaded too.
 */
class RetainedMethodSet {
private:
    // The key is the unloadingKey() of the method.
    //
    // If any insertion into a MethodMap fails, then it already contains an
    // equivalent entry, so we'll choose arbitrarily to keep the existing entry.
    //
    typedef TR::map<void *, TR_ResolvedMethod *> MethodMap;

public:
    /**
     * \brief Construct a RetainedMethodSet.
     *
     * \param comp the compilation object
     * \param method the assumed-live method
     * \param parent the parent RetainedMethodSet
     */
    RetainedMethodSet(TR::Compilation *comp, TR_ResolvedMethod *method, OMR::RetainedMethodSet *parent);

    /**
     * \brief Get the assumed-live method for this set.
     * \return the method
     */
    TR_ResolvedMethod *method() { return _method; }

    /**
     * \brief Determine whether \p method is guaranteed to remain loaded.
     *
     * A positive result (true) is reliable, but because this set is generally
     * incomplete, a negative result (false) could be spurious.
     *
     * The default implementation always gives a positive result.
     *
     * \param method the method in question
     * \return true if it is known to remain loaded, false otherwise
     */
    virtual bool willRemainLoaded(TR_ResolvedMethod *method) { return true; }

    /**
     * \brief Inspect the call site corresponding to \p bci in the bytecode and
     * attempt to return an extended version of this set if possible.
     *
     * Because this set is generally incomplete, there may be methods that will
     * in fact remain loaded but that are not yet known to do so, and some such
     * methods may be evident when inspecting the call site. If such a situation
     * is detected, this method will return a child set that takes those methods
     * into account.
     *
     * The default implementation returns the receiver.
     *
     * \param bci bytecode info of the call site to inspect
     */
    virtual OMR::RetainedMethodSet *withLinkedCalleeAttested(TR_ByteCodeInfo bci);

    /**
     * \brief Commit to keep alive this set's assumed-live method.
     */
    virtual void keepalive();

    /**
     * \brief Commit to invalidate the JIT body if this set's assumed-live
     * method is unloaded.
     */
    virtual void bond();

    /**
     * \brief Create a new child of this RetainedMethodSet with \p method as its
     * assumed-live method.
     *
     * Subclasses should override this to instantiate the appropriate type (most
     * likely the overriding subclass itself).
     *
     * This method must only be called in cases where this RetainedMethodSet
     * does not guarantee that \p method will remain loaded. Overrides may lift
     * this restriction. The default implementation is an "unimplemented"
     * assertion.
     *
     * \param method the assumed-live method
     * \return a new instance of RetainedMethodSet
     */
    virtual OMR::RetainedMethodSet *createChild(TR_ResolvedMethod *method);

    /**
     * \brief Get a RetainedMethodSet representing all methods known to remain
     * loaded in this set and also all of this set's keepalives.
     *
     * The result could be the receiver or a child set. The default
     * implementation simply returns the receiver.
     *
     * \return a version of this set with the keepalives attested to remain loaded.
     */
    virtual OMR::RetainedMethodSet *withKeepalivesAttested();

    /**
     * \brief Get a RetainedMethodSet representing all methods known to remain
     * loaded in this set and also all of this set's bonds.
     *
     * The result could be the receiver or a child set. The default
     * implementation simply returns the receiver.
     *
     * \return a version of this set with the bonds attested to remain loaded.
     */
    virtual OMR::RetainedMethodSet *withBondsAttested();

    /**
     * \brief An iterator that yields some number of TR_ResolvedMethod pointers.
     */
    class ResolvedMethodIter {
    private:
        friend class OMR::RetainedMethodSet;

        ResolvedMethodIter(const MethodMap &map)
            : _cur(map.begin())
            , _end(map.end())
        {}

    public:
        /**
         * \brief Get the next TR_ResolvedMethod if there is one.
         * \param[out] out the resulting TR_ReslovedMethod
         * \return true if \p out was set to the next value, or false if there
         *         were no remaining values
         */
        bool next(TR_ResolvedMethod **out)
        {
            if (_cur == _end) {
                *out = NULL;
                return false;
            } else {
                *out = _cur->second;
                _cur++;
                return true;
            }
        }

    private:
        MethodMap::const_iterator _cur;
        MethodMap::const_iterator _end;
    };

    /**
     * \brief Get the methods to keep alive at least as long as the JIT body.
     *
     * These are all of the keepalives generated by any set within the same
     * tree, i.e. any set descended from the same root set.
     *
     * \return an iterator that yields the methods to keep alive
     */
    ResolvedMethodIter keepaliveMethods() { return ResolvedMethodIter(keepalivesAndBonds()->_keepaliveMethods); }

    /**
     * \brief Get the methods to bond (limit the lifetime of the JIT body).
     *
     * These are all of the bonds generated by any set within the same tree,
     * i.e. any set descended from the same root set.
     *
     * \return an iterator that yields the methods to bond
     */
    ResolvedMethodIter bondMethods() { return ResolvedMethodIter(keepalivesAndBonds()->_bondMethods); }

protected:
    // The sets of keepalive and bond methods determined by analysis of inlining.
    // One instance of this is shared between all sets in a tree.
    struct KeepalivesAndBonds {
        MethodMap _keepaliveMethods;
        MethodMap _bondMethods;

        KeepalivesAndBonds(TR::Region &heapRegion);
    };

    KeepalivesAndBonds *keepalivesAndBonds();
    virtual KeepalivesAndBonds *createKeepalivesAndBonds();

    /**
     * \brief Determine the unloading key of \p method.
     *
     * For any two methods with the same unloading key, if one of the methods is
     * later unloaded, the other must be unloaded along with it. There are no
     * other restrictions on the value of the unloading key.
     *
     * The default implementation returns getNonPersistentIdentifier(), which is
     * always a safe choice because any two TR_ResolvedMethod instances that
     * share that value already fundamentally represent the same method.
     *
     * Subclasses can override this to provide a coarser partition as long as it
     * satisfies the property stated above. A coarser partition will result in
     * less redundancy in MethodMap.
     *
     * \param method the method
     * \return the unloading key
     */
    virtual void *unloadingKey(TR_ResolvedMethod *method);

    TR::Compilation *comp() { return _comp; }

    OMR::RetainedMethodSet *parent() { return _parent; }

private:
    void traceCommitment(const char *kind, void *key);

    TR::Compilation * const _comp;
    TR_ResolvedMethod * const _method;
    OMR::RetainedMethodSet * const _parent;

    // This is non-const because of lazy initialization in the root set.
    KeepalivesAndBonds *_keepalivesAndBonds;
};

} // namespace OMR

#endif // OMR_RETAINEDMETHODSET_INCL
