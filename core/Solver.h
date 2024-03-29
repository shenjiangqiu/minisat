/****************************************************************************************[Solver.h]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Minisat_Solver_h
#define Minisat_Solver_h

#include "mtl/Vec.h"
#include "mtl/Heap.h"
#include "mtl/Alg.h"
#include "utils/Options.h"
#include "core/SolverTypes.h"
extern Minisat::Int64Option opt_end_prop;
extern uint64_t next_print;

namespace Minisat
{

    class Solver;

    //=================================================================================================
    // Solver -- the main class:

    class Solver
    {

    public:
        int curr_restarts = 0; // function stack
        lbool curr_stats = l_Undef;
        double curr_rest_base;
        int curr_backtrack_level;
        int curr_conflictC = 0;
        vec<Lit> curr_learnt_clause;
        //number of decisions
        unsigned long long total_prop = 0;
        unsigned long long total_warmup = 0;
        bool finished_init = false;
        bool finished_warmup = false;
        int start_size = 0;
        int end_size = 0;
        //the simulation halt decision number, when this->prop>end_prop, it will halt. note: end_prop will be a absolute number,
        //when using checkpoint, remember to add the original start prop.
        unsigned long long end_prop = 0;
        bool operator==(const Solver &other)
        {
            if (total_prop == other.total_prop and
                total_warmup == other.total_warmup and
                finished_init == other.finished_init and
                finished_warmup == other.finished_warmup and
                started == other.started and
                model == other.model and
                conflict == other.conflict and
                verbosity == other.verbosity and
                var_decay == other.var_decay and
                clause_decay == other.clause_decay and
                random_var_freq == other.random_var_freq and
                random_seed == other.random_seed and
                luby_restart == other.luby_restart and
                ccmin_mode == other.ccmin_mode and
                phase_saving == other.phase_saving and
                rnd_pol == other.rnd_pol and
                rnd_init_act == other.rnd_init_act and
                garbage_frac == other.garbage_frac and
                restart_first == other.restart_first and
                restart_inc == other.restart_inc and
                learntsize_factor == other.learntsize_factor and
                learntsize_inc == other.learntsize_inc and
                learntsize_adjust_start_confl == other.learntsize_adjust_start_confl and
                learntsize_adjust_inc == other.learntsize_adjust_inc and
                solves == other.solves and
                starts == other.starts and
                decisions == other.decisions and
                rnd_decisions == other.rnd_decisions and
                propagations == other.propagations and
                conflicts == other.conflicts and
                dec_vars == other.dec_vars and
                clauses_literals == other.clauses_literals and
                learnts_literals == other.learnts_literals and
                max_literals == other.max_literals and
                tot_literals == other.tot_literals and
                ok == other.ok and
                clauses == other.clauses and
                learnts == other.learnts and
                cla_inc == other.cla_inc and
                activity == other.activity and
                var_inc == other.var_inc and
                watches == other.watches and
                assigns == other.assigns and
                polarity == other.polarity and
                decision == other.decision and
                trail == other.trail and
                trail_lim == other.trail_lim and
                vardata == other.vardata and
                qhead == other.qhead and
                simpDB_assigns == other.simpDB_assigns and
                simpDB_props == other.simpDB_props and
                assumptions == other.assumptions and
                order_heap == other.order_heap and
                progress_estimate == other.progress_estimate and
                remove_satisfied == other.remove_satisfied and
                ca == other.ca and
                seen == other.seen and
                analyze_stack == other.analyze_stack and
                analyze_toclear == other.analyze_toclear and
                add_tmp == other.add_tmp and
                max_learnts == other.max_learnts and
                learntsize_adjust_confl == other.learntsize_adjust_confl and
                learntsize_adjust_cnt == other.learntsize_adjust_cnt and
                conflict_budget == other.conflict_budget and
                propagation_budget == other.propagation_budget and
                asynch_interrupt == other.asynch_interrupt)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        template <class Archive>
        void serialize(Archive &ar, const unsigned int)
        {

            //stack infomation
            ar &curr_restarts;
            ar &curr_stats;
            ar &curr_rest_base;
            ar &curr_backtrack_level;
            ar &curr_conflictC;
            ar &curr_learnt_clause;

            ar &start_size;
            ar &end_size;
            //currently the number of decisions
            ar &total_prop;
            ar &total_warmup;
            ar &finished_init;
            ar &finished_warmup;
            ar &started;
            ar &model;
            ar &conflict;
            ar &verbosity;
            ar &var_decay;
            ar &clause_decay;
            ar &random_var_freq;
            ar &random_seed;
            ar &luby_restart;
            ar &ccmin_mode;
            ar &phase_saving;
            ar &rnd_pol;
            ar &rnd_init_act;
            ar &garbage_frac;
            ar &restart_first;
            ar &restart_inc;
            ar &learntsize_factor;
            ar &learntsize_inc;
            ar &learntsize_adjust_start_confl;
            ar &learntsize_adjust_inc;
            ar &solves;
            ar &starts;
            ar &decisions;
            ar &rnd_decisions;
            ar &propagations;
            ar &conflicts;
            ar &dec_vars;
            ar &clauses_literals;
            ar &learnts_literals;
            ar &max_literals;
            ar &tot_literals;
            ar &ok;
            ar &clauses;
            ar &learnts;
            ar &cla_inc;
            ar &activity;
            ar &var_inc;
            ar &watches;
            ar &assigns;
            ar &polarity;
            ar &decision;
            ar &trail;
            ar &trail_lim;
            ar &vardata;
            ar &qhead;
            ar &simpDB_assigns;
            ar &simpDB_props;
            ar &assumptions;
            ar &order_heap;
            ar &progress_estimate;
            ar &remove_satisfied;
            ar &ca;
            ar &seen;
            ar &analyze_stack;
            ar &analyze_toclear;
            ar &add_tmp;
            ar &max_learnts;
            ar &learntsize_adjust_confl;
            ar &learntsize_adjust_cnt;
            ar &conflict_budget;
            ar &propagation_budget;
            ar &asynch_interrupt;
            end_prop = opt_end_prop;
            next_print = propagations;
            first_prop=propagations;
        }

        bool started = false;
        // Constructor/Destructor:
        //
        Solver();
        virtual ~Solver();

        // Problem specification:
        //
        Var newVar(bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.

        bool addClause(const vec<Lit> &ps);  // Add a clause to the solver.
        bool addEmptyClause();               // Add the empty clause, making the solver contradictory.
        bool addClause(Lit p);               // Add a unit clause to the solver.
        bool addClause(Lit p, Lit q);        // Add a binary clause to the solver.
        bool addClause(Lit p, Lit q, Lit r); // Add a ternary clause to the solver.
        bool addClause_(vec<Lit> &ps);       // Add a clause to the solver without making superflous internal copy. Will
                                             // change the passed vector 'ps'.

        // Solving:
        //
        bool simplify();                             // Removes already satisfied clauses.
        bool solve(const vec<Lit> &assumps);         // Search for a model that respects a given set of assumptions.
        lbool solveLimited(const vec<Lit> &assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
        bool solve();                                // Search without assumptions.
        bool solve(Lit p);                           // Search for a model that respects a single assumption.
        bool solve(Lit p, Lit q);                    // Search for a model that respects two assumptions.
        bool solve(Lit p, Lit q, Lit r);             // Search for a model that respects three assumptions.
        bool okay() const;                           // FALSE means solver is in a conflicting state

        void toDimacs(FILE *f, const vec<Lit> &assumps); // Write CNF to file in DIMACS-format.
        void toDimacs(const char *file, const vec<Lit> &assumps);
        void toDimacs(FILE *f, Clause &c, vec<Var> &map, Var &max);

        // Convenience versions of 'toDimacs()':
        void toDimacs(const char *file);
        void toDimacs(const char *file, Lit p);
        void toDimacs(const char *file, Lit p, Lit q);
        void toDimacs(const char *file, Lit p, Lit q, Lit r);

        // Variable mode:
        //
        void setPolarity(Var v, bool b);    // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
        void setDecisionVar(Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

        // Read state:
        //
        lbool value(Var x) const;      // The current value of a variable.
        lbool value(Lit p) const;      // The current value of a literal.
        lbool modelValue(Var x) const; // The value of a variable in the last model. The last call to solve must have been satisfiable.
        lbool modelValue(Lit p) const; // The value of a literal in the last model. The last call to solve must have been satisfiable.
        int nAssigns() const;          // The current number of assigned literals.
        int nClauses() const;          // The current number of original clauses.
        int nLearnts() const;          // The current number of learnt clauses.
        int nVars() const;             // The current number of variables.
        int nFreeVars() const;

        // Resource contraints:
        //
        void setConfBudget(int64_t x);
        void setPropBudget(int64_t x);
        void budgetOff();
        void interrupt();      // Trigger a (potentially asynchronous) interruption of the solver.
        void clearInterrupt(); // Clear interrupt indicator flag.

        // Memory managment:
        //
        virtual void garbageCollect();
        void checkGarbage(double gf);
        void checkGarbage();

        // Extra results: (read-only member variable)
        //
        vec<lbool> model;  // If problem is satisfiable, this vector contains the model (if any).
        vec<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions),
                           // this vector represent the final conflict clause expressed in the assumptions.

        // Mode of operation:
        //
        int verbosity;
        double var_decay;
        double clause_decay;
        double random_var_freq;
        double random_seed;
        bool luby_restart;
        int ccmin_mode;      // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
        int phase_saving;    // Controls the level of phase saving (0=none, 1=limited, 2=full).
        bool rnd_pol;        // Use random polarities for branching heuristics.
        bool rnd_init_act;   // Initialize variable activities with a small random value.
        double garbage_frac; // The fraction of wasted memory allowed before a garbage collection is triggered.

        int restart_first;        // The initial restart limit.                                                                (default 100)
        double restart_inc;       // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
        double learntsize_factor; // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
        double learntsize_inc;    // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)

        int learntsize_adjust_start_confl;
        double learntsize_adjust_inc;

        // Statistics: (read-only member variable)
        //
        uint64_t solves, starts, decisions, rnd_decisions, first_prop,/*number of propagations*/propagations, conflicts;
        uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;
        struct VarData
        {
            bool operator==(const VarData &other)
            {
                if (reason == other.reason and level == other.level)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
            template <class Archive>
            void serialize(Archive &ar, const unsigned int)
            {
                ar &reason;
                ar &level;
            }

            CRef reason;
            int level;
        };
        struct Watcher
        {
            template <class Archive>
            void serialize(Archive &ar, const unsigned int)
            {
                ar &cref;
                ar &blocker;
            }
            CRef cref;
            Lit blocker;
            Watcher(CRef cr, Lit p) : cref(cr), blocker(p) {}
            bool operator==(const Watcher &w) const { return cref == w.cref; }
            bool operator!=(const Watcher &w) const { return cref != w.cref; }
        };
        struct WatcherDeleted
        {
            const ClauseAllocator &ca;
            WatcherDeleted(const ClauseAllocator &_ca) : ca(_ca) {}
            bool operator()(const Watcher &w) const { return ca[w.cref].mark() == 1; }
        };
        OccLists<Lit, vec<Watcher>, WatcherDeleted>
            watches; // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    protected:
        // Helper structures:
        //

        static inline VarData mkVarData(CRef cr, int l)
        {
            VarData d = {cr, l};
            return d;
        }

        struct VarOrderLt
        {
            const vec<double> &activity;
            bool operator()(Var x, Var y) const { return activity[x] > activity[y]; }
            VarOrderLt(const vec<double> &act) : activity(act) {}
        };

        // Solver state:
        //
        bool ok;              // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
        vec<CRef> clauses;    // List of problem clauses.
        vec<CRef> learnts;    // List of learnt clauses.
        double cla_inc;       // Amount to bump next clause with.
        vec<double> activity; // A heuristic measurement of the activity of a variable.
        double var_inc;       // Amount to bump next variable with.

        vec<lbool> assigns;          // The current assignments.
        vec<char> polarity;          // The preferred polarity of each variable.
        vec<char> decision;          // Declares if a variable is eligible for selection in the decision heuristic.
        vec<Lit> trail;              // Assignment stack; stores all assigments made in the order they were made.
        vec<int> trail_lim;          // Separator indices for different decision levels in 'trail'.
        vec<VarData> vardata;        // Stores reason and level for each variable.
        int qhead;                   // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
        int simpDB_assigns;          // Number of top-level assignments since last execution of 'simplify()'.
        int64_t simpDB_props;        // Remaining number of propagations that must be made before next execution of 'simplify()'.
        vec<Lit> assumptions;        // Current set of assumptions provided to solve by the user.
        Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
        double progress_estimate;    // Set by 'search()'.
        bool remove_satisfied;       // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.

        ClauseAllocator ca;

        // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
        // used, exept 'seen' wich is used in several places.
        //
        vec<char> seen;
        vec<Lit> analyze_stack;
        vec<Lit> analyze_toclear;
        vec<Lit> add_tmp;

        double max_learnts;
        double learntsize_adjust_confl;
        int learntsize_adjust_cnt;

        // Resource contraints:
        //
        int64_t conflict_budget;    // -1 means no budget.
        int64_t propagation_budget; // -1 means no budget.
        bool asynch_interrupt;

        // Main internal methods:
        //
        void insertVarOrder(Var x);                                       // Insert a variable in the decision order priority queue.
        Lit pickBranchLit();                                              // Return the next decision variable.
        void newDecisionLevel();                                          // Begins a new decision level.
        void uncheckedEnqueue(Lit p, CRef from = CRef_Undef);             // Enqueue a literal. Assumes value of literal is undefined.
        bool enqueue(Lit p, CRef from = CRef_Undef);                      // Test if fact 'p' contradicts current state, enqueue otherwise.
        CRef propagate();                                                 // Perform unit propagation. Returns possibly conflicting clause.
        void cancelUntil(int level);                                      // Backtrack until a certain level.
        void analyze(CRef confl, vec<Lit> &out_learnt, int &out_btlevel); // (bt = backtrack)
        void analyzeFinal(Lit p, vec<Lit> &out_conflict);                 // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
        bool litRedundant(Lit p, uint32_t abstract_levels);               // (helper method for 'analyze()')
        lbool search(int nof_conflicts);                                  // Search for a given number of conflicts.
        lbool solve_();                                                   // Main solve method (assumptions given in 'assumptions').
        void reduceDB();                                                  // Reduce the set of learnt clauses.
        void removeSatisfied(vec<CRef> &cs);                              // Shrink 'cs' to contain only non-satisfied clauses.
        void rebuildOrderHeap();

        // Maintaining Variable/Clause activity:
        //
        void varDecayActivity();                 // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
        void varBumpActivity(Var v, double inc); // Increase a variable with the current 'bump' value.
        void varBumpActivity(Var v);             // Increase a variable with the current 'bump' value.
        void claDecayActivity();                 // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
        void claBumpActivity(Clause &c);         // Increase a clause with the current 'bump' value.

        // Operations on clauses:
        //
        void attachClause(CRef cr);                      // Attach a clause to watcher lists.
        void detachClause(CRef cr, bool strict = false); // Detach a clause to watcher lists.
        void removeClause(CRef cr);                      // Detach and free a clause.
        bool locked(const Clause &c) const;              // Returns TRUE if a clause is a reason for some implication in the current state.
        bool satisfied(const Clause &c) const;           // Returns TRUE if a clause is satisfied in the current state.

        void relocAll(ClauseAllocator &to);

        // Misc:
        //
        int decisionLevel() const;           // Gives the current decisionlevel.
        uint32_t abstractLevel(Var x) const; // Used to represent an abstraction of sets of decision levels.
        CRef reason(Var x) const;
        int level(Var x) const;
        double progressEstimate() const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
        bool withinBudget() const;

        // Static helpers:
        //

        // Returns a random float 0 <= x < 1. Seed must never be 0.
        static inline double drand(double &seed)
        {
            seed *= 1389796;
            int q = (int)(seed / 2147483647);
            seed -= (double)q * 2147483647;
            return seed / 2147483647;
        }

        // Returns a random integer 0 <= x < size. Seed must never be 0.
        static inline int irand(double &seed, int size)
        {
            return (int)(drand(seed) * size);
        }
    };

    //=================================================================================================
    // Implementation of inline methods:

    inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
    inline int Solver::level(Var x) const { return vardata[x].level; }

    inline void Solver::insertVarOrder(Var x)
    {
        if (!order_heap.inHeap(x) && decision[x])
            order_heap.insert(x);
    }

    inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }
    inline void Solver::varBumpActivity(Var v) { varBumpActivity(v, var_inc); }
    inline void Solver::varBumpActivity(Var v, double inc)
    {
        if ((activity[v] += inc) > 1e100)
        {
            // Rescale:
            for (int i = 0; i < nVars(); i++)
                activity[i] *= 1e-100;
            var_inc *= 1e-100;
        }

        // Update order_heap with respect to new activity:
        if (order_heap.inHeap(v))
            order_heap.decrease(v);
    }

    inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
    inline void Solver::claBumpActivity(Clause &c)
    {
        if ((c.activity() += cla_inc) > 1e20)
        {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                ca[learnts[i]].activity() *= 1e-20;
            cla_inc *= 1e-20;
        }
    }

    inline void Solver::checkGarbage(void) { return checkGarbage(garbage_frac); }
    inline void Solver::checkGarbage(double gf)
    {
        if (ca.wasted() > ca.size() * gf)
            garbageCollect();
    }

    // NOTE: enqueue does not set the ok flag! (only public methods do)
    inline bool Solver::enqueue(Lit p, CRef from) { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
    inline bool Solver::addClause(const vec<Lit> &ps)
    {
        ps.copyTo(add_tmp);
        return addClause_(add_tmp);
    }
    inline bool Solver::addEmptyClause()
    {
        add_tmp.clear();
        return addClause_(add_tmp);
    }
    inline bool Solver::addClause(Lit p)
    {
        add_tmp.clear();
        add_tmp.push(p);
        return addClause_(add_tmp);
    }
    inline bool Solver::addClause(Lit p, Lit q)
    {
        add_tmp.clear();
        add_tmp.push(p);
        add_tmp.push(q);
        return addClause_(add_tmp);
    }
    inline bool Solver::addClause(Lit p, Lit q, Lit r)
    {
        add_tmp.clear();
        add_tmp.push(p);
        add_tmp.push(q);
        add_tmp.push(r);
        return addClause_(add_tmp);
    }
    inline bool Solver::locked(const Clause &c) const { return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c; }
    inline void Solver::newDecisionLevel() { trail_lim.push(trail.size()); }

    inline int Solver::decisionLevel() const { return trail_lim.size(); }
    inline uint32_t Solver::abstractLevel(Var x) const { return 1 << (level(x) & 31); }
    inline lbool Solver::value(Var x) const { return assigns[x]; }
    inline lbool Solver::value(Lit p) const { return assigns[var(p)] ^ sign(p); }
    inline lbool Solver::modelValue(Var x) const { return model[x]; }
    inline lbool Solver::modelValue(Lit p) const { return model[var(p)] ^ sign(p); }
    inline int Solver::nAssigns() const { return trail.size(); }
    inline int Solver::nClauses() const { return clauses.size(); }
    inline int Solver::nLearnts() const { return learnts.size(); }
    inline int Solver::nVars() const { return vardata.size(); }
    inline int Solver::nFreeVars() const { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
    inline void Solver::setPolarity(Var v, bool b) { polarity[v] = b; }
    inline void Solver::setDecisionVar(Var v, bool b)
    {
        if (b && !decision[v])
            dec_vars++;
        else if (!b && decision[v])
            dec_vars--;

        decision[v] = b;
        insertVarOrder(v);
    }
    inline void Solver::setConfBudget(int64_t x) { conflict_budget = conflicts + x; }
    inline void Solver::setPropBudget(int64_t x) { propagation_budget = propagations + x; }
    inline void Solver::interrupt() { asynch_interrupt = true; }
    inline void Solver::clearInterrupt() { asynch_interrupt = false; }
    inline void Solver::budgetOff() { conflict_budget = propagation_budget = -1; }
    inline bool Solver::withinBudget() const
    {
        return !asynch_interrupt &&
               (conflict_budget < 0 || conflicts < (uint64_t)conflict_budget) &&
               (propagation_budget < 0 || propagations < (uint64_t)propagation_budget);
    }

    // FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
    // pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
    // all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
    inline bool Solver::solve()
    {
        budgetOff();
        assumptions.clear();
        return solve_() == l_True;
    }
    inline bool Solver::solve(Lit p)
    {
        budgetOff();
        assumptions.clear();
        assumptions.push(p);
        return solve_() == l_True;
    }
    inline bool Solver::solve(Lit p, Lit q)
    {
        budgetOff();
        assumptions.clear();
        assumptions.push(p);
        assumptions.push(q);
        return solve_() == l_True;
    }
    inline bool Solver::solve(Lit p, Lit q, Lit r)
    {
        budgetOff();
        assumptions.clear();
        assumptions.push(p);
        assumptions.push(q);
        assumptions.push(r);
        return solve_() == l_True;
    }
    inline bool Solver::solve(const vec<Lit> &assumps)
    {
        budgetOff();
        assumps.copyTo(assumptions);
        return solve_() == l_True;
    }
    inline lbool Solver::solveLimited(const vec<Lit> &assumps)
    {
        assumps.copyTo(assumptions);
        return solve_();
    }
    inline bool Solver::okay() const { return ok; }

    inline void Solver::toDimacs(const char *file)
    {
        vec<Lit> as;
        toDimacs(file, as);
    }
    inline void Solver::toDimacs(const char *file, Lit p)
    {
        vec<Lit> as;
        as.push(p);
        toDimacs(file, as);
    }
    inline void Solver::toDimacs(const char *file, Lit p, Lit q)
    {
        vec<Lit> as;
        as.push(p);
        as.push(q);
        toDimacs(file, as);
    }
    inline void Solver::toDimacs(const char *file, Lit p, Lit q, Lit r)
    {
        vec<Lit> as;
        as.push(p);
        as.push(q);
        as.push(r);
        toDimacs(file, as);
    }

    //=================================================================================================
    // Debug etc:

    //=================================================================================================
} // namespace Minisat

#endif
