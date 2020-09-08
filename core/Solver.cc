/***************************************************************************************[Solver.cc]
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

#include <math.h>
//#include <core/acc.h>
//#include <core/assign_wrap.h>

#include "mtl/Sort.h"
#include "core/Solver.h"
#include "acc.h"
#include "core/read_config.h"
#include <fstream>
#include <map>
using namespace Minisat;
#include "core/cache_wrap.h"
//=================================================================================================
// Options:
#include <chrono>
using namespace std::chrono;
static const char *_cat = "CORE";

static DoubleOption opt_var_decay(_cat, "var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
static DoubleOption opt_clause_decay(_cat, "cla-decay", "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));
static DoubleOption opt_random_var_freq(_cat, "rnd-freq", "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption opt_random_seed(_cat, "rnd-seed", "Used by the random variable selection", 91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption opt_ccmin_mode(_cat, "ccmin-mode", "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption opt_phase_saving(_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption opt_rnd_init_act(_cat, "rnd-init", "Randomize the initial activity", false);
static BoolOption opt_luby_restart(_cat, "luby", "Use the Luby restart sequence", true);
static IntOption opt_restart_first(_cat, "rfirst", "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption opt_restart_inc(_cat, "rinc", "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption opt_garbage_frac(_cat, "gc-frac", "The fraction of wasted memory allowed before a garbage collection is triggered", 0.20, DoubleRange(0, false, HUGE_VAL, false));

//=================================================================================================
// Constructor/Destructor:
// sjq options

static BoolOption opt_enable_acc("sjq", "enable-acc", "weather enable simulation(for acc only)", false);

static Int64Option opt_warmup_prop("sjq", "warmup-prop", "props to warmup", 0);
static Int64Option opt_end_prop("sjq", "end-prop", "props to end", 0);
BoolOption opt_save("sjq", "save", "weather save the checkpoint", false);
BoolOption opt_load("sjq", "load", "weather load the checkpoint", false);
Int64Option opt_checkpoint_prop("sjq", "checkpoint-prop", "when to save checkpoint");
StringOption opt_checkpoint_name("sjq", "checkpoint-name", "the name of checkpoint");
BoolOption opt_seq("sjq", "seq", "weather get sequential time", false);

// end sjq options

Solver::Solver() :

                   // Parameters (user settable):
                   //
                   verbosity(0), var_decay(opt_var_decay), clause_decay(opt_clause_decay), random_var_freq(opt_random_var_freq), random_seed(opt_random_seed), luby_restart(opt_luby_restart), ccmin_mode(opt_ccmin_mode), phase_saving(opt_phase_saving), rnd_pol(false), rnd_init_act(opt_rnd_init_act), garbage_frac(opt_garbage_frac), restart_first(opt_restart_first), restart_inc(opt_restart_inc)

                   // Parameters (the rest):
                   //
                   ,
                   learntsize_factor((double)1 / (double)3), learntsize_inc(1.1)

                   // Parameters (experimental):
                   //
                   ,
                   learntsize_adjust_start_confl(100), learntsize_adjust_inc(1.5)

                   // Statistics: (formerly in 'SolverStats')
                   //
                   ,
                   solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0), dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

                   ,
                   watches(WatcherDeleted(ca)), ok(true), cla_inc(1), var_inc(1), qhead(0), simpDB_assigns(-1), simpDB_props(0), order_heap(VarOrderLt(activity)), progress_estimate(0), remove_satisfied(true)

                   // Resource constraints:
                   //
                   ,
                   conflict_budget(-1), propagation_budget(-1), asynch_interrupt(false)
{
    finished_warmup = opt_warmup_prop > 0 ? false : true;
}

Solver::~Solver()
{
}

//=================================================================================================
// Minor methods:

// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches.init(mkLit(v, false));
    watches.init(mkLit(v, true));
    assigns.push(l_Undef);
    vardata.push(mkVarData(CRef_Undef, 0));
    //activity .push(0);
    activity.push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen.push(0);
    polarity.push(sign);
    decision.push();
    trail.capacity(v + 1);
    setDecisionVar(v, dvar);
    return v;
}

bool Solver::addClause_(vec<Lit> &ps)
{
    assert(decisionLevel() == 0);
    if (!ok)
        return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    ps.shrink(i - j);

    if (ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1)
    {
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    }
    else
    {
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);
    }

    return true;
}

void Solver::attachClause(CRef cr)
{
    const Clause &c = ca[cr];
    assert(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt())
        learnts_literals += c.size();
    else
        clauses_literals += c.size();
}

void Solver::detachClause(CRef cr, bool strict)
{
    const Clause &c = ca[cr];
    assert(c.size() > 1);

    if (strict)
    {
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    }
    else
    {
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if (c.learnt())
        learnts_literals -= c.size();
    else
        clauses_literals -= c.size();
}

void Solver::removeClause(CRef cr)
{
    Clause &c = ca[cr];
    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(c))
        vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);
    ca.free(cr);
}

bool Solver::satisfied(const Clause &c) const
{
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false;
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level)
{
    if (decisionLevel() > level)
    {
        for (int c = trail.size() - 1; c >= trail_lim[level]; c--)
        {
            Var x = var(trail[c]);
            assigns[x] = l_Undef;
            if (phase_saving > 1 || ((phase_saving == 1) && c > trail_lim.last()))
                polarity[x] = sign(trail[c]);
            insertVarOrder(x);
        }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }
}

//=================================================================================================
// Major methods:

Lit Solver::pickBranchLit()
{
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty())
    {
        next = order_heap[irand(random_seed, order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++;
    }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty())
        {
            next = var_Undef;
            break;
        }
        else
            next = order_heap.removeMin();

    return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}

/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|  
|  Description:
|    Analyze conflict and produce a reason clause.
|  
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|  
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the 
|        rest of literals. There may be others from the same level though.
|  
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit> &out_learnt, int &out_btlevel)
{
    int pathC = 0;
    Lit p = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push(); // (leave room for the asserting literal)
    int index = trail.size() - 1;

    do
    {
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause &c = ca[confl];

        if (c.learnt())
            claBumpActivity(c);

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++)
        {
            Lit q = c[j];

            if (!seen[var(q)] && level(var(q)) > 0)
            {
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                if (level(var(q)) >= decisionLevel())
                    pathC++;
                else
                    out_learnt.push(q);
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])])
            ;
        p = trail[index + 1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    } while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2)
    {
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
    }
    else if (ccmin_mode == 1)
    {
        for (i = j = 1; i < out_learnt.size(); i++)
        {
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else
            {
                Clause &c = ca[reason(var(out_learnt[i]))];
                for (int k = 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0)
                    {
                        out_learnt[j++] = out_learnt[i];
                        break;
                    }
            }
        }
    }
    else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else
    {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1] = p;
        out_btlevel = level(var(p));
    }

    for (int j = 0; j < analyze_toclear.size(); j++)
        seen[var(analyze_toclear[j])] = 0; // ('seen[]' is now cleared)
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0)
    {
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause &c = ca[reason(var(analyze_stack.last()))];
        analyze_stack.pop();

        for (int i = 1; i < c.size(); i++)
        {
            Lit p = c[i];
            if (!seen[var(p)] && level(var(p)) > 0)
            {
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0)
                {
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }
                else
                {
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}

/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|  
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit> &out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size() - 1; i >= trail_lim[0]; i--)
    {
        Var x = var(trail[i]);
        if (seen[x])
        {
            if (reason(x) == CRef_Undef)
            {
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }
            else
            {
                Clause &c = ca[reason(x)];
                for (int j = 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::uncheckedEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|  
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|  
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
using value_type = int;
std::vector<acc *> m_accs;
std::vector<uint64_t> current_cycle_s;
auto &get_acc()
{
    if (!m_accs.empty())
    {
        return m_accs;
    }
    else
    {
        //current_cycle_s.push_back(0);
        current_cycle_s.push_back(0);
        current_cycle_s.push_back(0);

        current_cycle_s.push_back(0);

        //m_accs.push_back(new acc(4, 4, current_cycle_s[0]));
        m_accs.push_back(new acc(16, 16, current_cycle_s[0]));
        m_accs.push_back(new acc(16, 32, current_cycle_s[1]));
        m_accs.push_back(new acc(16, 64, current_cycle_s[2]));
        return m_accs;
    }
}
CacheWrap m_cache_wrap;

unsigned long long total_cycle_in_bcp_sq = 0;

void accumulate(unsigned long long &to_be_accumulated, CacheWrap &cache, void *addr, int type)
{
    auto result = cache.access((unsigned long long)addr, type);
    if (result.first == CacheWrap::hit)
    {

        switch (result.second)
        {
        case CacheWrap::L1:
            to_be_accumulated += 1;
            break;
        case CacheWrap::L2:
            to_be_accumulated += 10;
            break;
        case CacheWrap::L3:
            to_be_accumulated += 60;
            break;
        }
    }
    else
    {

        assert(result.first == CacheWrap::miss);
        to_be_accumulated += 119;
    }
}
assign_wrap_factory awf;
CRef Solver::propagate()
{
    //std::ofstream real("real.txt", std::ios_base::app);

    //SimMarker(CONTROL_MAGIC_A,CONTROL_PROP_START_B);

    //std::map<int, int> generate_relation_map;
    if (opt_seq and finished_warmup and total_prop % 10000 == 1)
    {
        std::cout << "total_prop " << total_prop << std::endl;
        std::cout << "total_cycle " << total_cycle_in_bcp_sq << std::endl;
    }

    std::map<int, assign_wrap *> lit_to_wrap;

    CRef confl = CRef_Undef;
    int num_props = 0;
    watches.cleanAll();

    //assign_wrap* shared_null;
    //std::unordered_map<unsigned long long, int> watcher_access;
    //std::unordered_map<unsigned long long, int> clause_access;
    assign_wrap *first_wrap = nullptr;
    while (qhead < trail.size())
    {
        Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.
        //std::cout << "minisat::lit: " << p.x << std::endl;
        vec<Watcher> &ws = watches[p];
        if (ws.size() == 0)
        {
            continue;
        }
        Watcher *i, *j, *end;
        num_props++;

        assign_wrap *this_wrap = nullptr;
        if (finished_warmup and opt_enable_acc)
        {
            bool first = lit_to_wrap.find(p.x) == lit_to_wrap.end();

            if (first)
            {
                this_wrap = awf.create(p.x, ws.size(), -1, nullptr, 0); // the first one
                first_wrap = this_wrap;
                lit_to_wrap[p.x] = this_wrap;
            }
            else
            {
                this_wrap = lit_to_wrap[p.x];
                this_wrap->set_watcher_size(ws.size());
            }

            this_wrap->set_addr((unsigned long long)((Watcher *)ws));
            //watcher_access[(unsigned long long)((Watcher *)ws)]++;
        }

        int ii = 0;
        for (i = j = (Watcher *)ws, end = i + ws.size(); i != end;)
        {

            ii++;
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (opt_seq and finished_warmup)
            {
                //simulate watcher read
                accumulate(total_cycle_in_bcp_sq, m_cache_wrap, i, 0);
                //simulate value read
                accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &assigns[var(blocker)], 0);
            }
            if (finished_warmup and opt_enable_acc)
                this_wrap->add_block_addr(ii - 1, (unsigned long long)(&assigns[var(blocker)]));
            //notice there, this is finished in the watcher unit,push it back to the current watcher list.
            if (value(blocker) == l_True)
            {
                if (opt_seq and finished_warmup)
                {
                    total_cycle_in_bcp_sq += 2;
                }
                *j++ = *i++;
                continue;
            }

            // Make sure the false literal is data[1]:

            CRef cr = i->cref;
            Clause &c = ca[cr];
            assert(&c == ca.lea(cr));
            if (finished_warmup and opt_enable_acc)
                this_wrap->add_clause_addr(ii - 1, (unsigned long long)(&(c.data))); //currently we don't care about the address//no we need it!!!!
            //clause_access[(unsigned long long)ca.lea(cr)]++;
            Lit false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;
            if (opt_seq and finished_warmup)
            {
                accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &ca[cr], 1);
                accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &c[0], 1);
                accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &c[1], 1);
            }
            // If 0th watch is true, then clause is already satisfied.
            Lit first = c[0];
            Watcher w = Watcher(cr, first);
            if (opt_seq and finished_warmup)
                total_cycle_in_bcp_sq += 2;
            if (finished_warmup and opt_enable_acc)
            {
                //std::cout<<ii-1<<std::endl;
                this_wrap->add_detail(ii - 1, (unsigned long long)(&assigns[var(c[0])]));
                this_wrap->add_detail(ii - 1, (unsigned long long)(&assigns[var(c[1])]));
                this_wrap->add_clause_literal(ii - 1, c[0]);
                this_wrap->add_clause_literal(ii - 1, c[1]);
            }
            if (first != blocker && value(first) == l_True)
            {
                if (opt_seq and finished_warmup)
                    total_cycle_in_bcp_sq += 2;
                *j++ = w;
                continue;
            }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
            {

                if (opt_seq and finished_warmup)
                {
                    accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &assigns[var(c[k])], 0);
                    accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &c[k], 1);
                }
                if (finished_warmup and opt_enable_acc)
                {
                    this_wrap->add_detail(ii - 1, (unsigned long long)(&assigns[var(c[k])]));
                    this_wrap->add_clause_literal(ii - 1, c[k]);
                }

                if (value(c[k]) != l_False)
                {
                    if (opt_seq and finished_warmup)
                        accumulate(total_cycle_in_bcp_sq, m_cache_wrap, &watches[~c[1]], 0);
                    c[1] = c[k];
                    c[k] = false_lit;
                    watches[~c[1]].push(w);
                    if (finished_warmup and opt_enable_acc)
                    {
                        this_wrap->add_pushed_list(ii - 1, int(~c[1]));
                        auto &pushed_watcher = watches[~c[1]];
                        auto &last_location = pushed_watcher[pushed_watcher.size() - 1];

                        this_wrap->add_pushed_addr(ii - 1, (unsigned long long)&last_location);
                    }
                    goto NextClause;
                }
            }
            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False)
            {
                if (finished_warmup and opt_enable_acc)
                    this_wrap->set_generated_conf(ii - 1);
                //get_acc()->set_ready();
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                if (finished_warmup and opt_enable_acc)
                    this_wrap->set_watcher_size(ii);
                while (i < end)
                    *j++ = *i++;
            }
            else
            {
                //first generate new wrap;
                // wrap size=10//that's a arbitrary value, cause we don't know it yet
                if (finished_warmup and opt_enable_acc)
                {
                    //watcher size should be 0 here, we might not access this any more.
                    auto new_wrap = awf.create(first, 0, ii - 1, this_wrap, this_wrap->get_level() + 1);
                    //init the block addr value, to avent segment fault
                    for (int i = 0; i < watches[first].size(); i++)
                    {
                        new_wrap->add_block_addr(i, 0);
                    }

                    lit_to_wrap.insert({first, new_wrap});
                }
                uncheckedEnqueue(first, cr);
            }

        NextClause:;
        }
        ws.shrink(i - j);
    } // end while (qhead < trail.size())
    propagations += num_props;
    simpDB_props -= num_props;

    //SimMarker(CONTROL_MAGIC_A, CONTROL_PROP_END_B);
    // now ready to sim
    //get_acc()->print_on(1);

    if (finished_warmup and opt_enable_acc)
    {
        if (first_wrap != nullptr)
            for (auto &&mc : get_acc())
            {
                mc->in_m_trail.push_back(std::make_unique<cache_interface_req>(ReadType::ReadWatcher, 0, 0, 0, first_wrap));
            }
        //std::vector<int> this_cycle;
        //std::cout<<"start!"<<total_prop<<std::endl;
        for (unsigned int i = 0; i < get_acc().size(); i++)
        {
            while (!get_acc()[i]->empty())
            {
                get_acc()[i]->cycle();
                get_acc()[i]->current_cycle++;
            }
        }
        //clean the evironment
        //maybe we need use unique_ptr? 
        for (auto value : lit_to_wrap)
        {
            delete value.second;
        }

        if (total_prop % 10000 == 1)
        {
            //std::for_each(get_acc().begin(), get_acc().end(), [](auto p_acc) { std::cout << *p_acc << std::endl; });
            for (unsigned int i = 0; i < get_acc().size(); i++)
            {
                std::cout << "\n\nprint the " << i << " th acc" << std::endl;
                std::cout << "total_prop: " << total_prop << std::endl;
                std::cout << "total_cycle: " << get_acc()[i]->current_cycle << std::endl;
                std::cout << get_acc()[i]->get_line_trace() << std::endl;
                end_size = ca.size();
                std::cout << "total_clause_size: " << end_size << std::endl;
                std::cout << "origin_clause_size: " << start_size << std::endl;
                std::cout << "origin_clause_num: " << clauses.size() << std::endl;
                std::cout << "learnt_clasue_num: " << learnts.size() << std::endl;
                //handle exit logic,
            }
        }
        //if (total_prop >= 3000000)
    }

    //std::cout << "total_prop:" << total_prop << std::endl;

    return confl;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|  
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt
{
    ClauseAllocator &ca;
    reduceDB_lt(ClauseAllocator &ca_) : ca(ca_) {}
    bool operator()(CRef x, CRef y)
    {
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity());
    }
};
void Solver::reduceDB()
{
    int i, j;
    double extra_lim = cla_inc / learnts.size(); // Remove any clause below this activity

    sort(learnts, reduceDB_lt(ca));
    // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
    // and clauses with activity smaller than 'extra_lim':
    for (i = j = 0; i < learnts.size(); i++)
    {
        Clause &c = ca[learnts[i]];
        if (c.size() > 2 && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim))
            removeClause(learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
    checkGarbage();
}

void Solver::removeSatisfied(vec<CRef> &cs)
{
    int i, j;
    for (i = j = 0; i < cs.size(); i++)
    {
        Clause &c = ca[cs[i]];
        if (satisfied(c))
            removeClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

void Solver::rebuildOrderHeap()
{
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|  
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok || propagate() != CRef_Undef)
        return ok = false;

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied) // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props = clauses_literals + learnts_literals; // (shouldn't depend on stats really, but it will do for now)

    return true;
}

/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|  
|  Description:
|    Search for a model the specified number of conflicts. 
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|  
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{

    static nanoseconds total_time_in_bcp(0);
    assert(ok);
    static bool first_in = true;
    if (!opt_load)
    {
        first_in = false;
    }
    if (!first_in)
    {
        curr_backtrack_level = 0;
        curr_conflictC = 0;
        curr_learnt_clause.clear();

        starts++;
    }
    for (;;)
    {
        if (opt_save)
        {
            if (total_prop >= (unsigned long long)opt_checkpoint_prop)
            {
                { //save to file now;
                    std::ofstream ofs(opt_checkpoint_name);
                    boost::archive::binary_oarchive oa(ofs);
                    oa &(*this);
                    std::cout << "save the checkpoint to file " << opt_checkpoint_name << std::endl;
                }
                exit(0);
            }
        }

        if (opt_end_prop > 0 and total_prop >= (unsigned long long)opt_end_prop)
        {

            end_size = ca.size();
            std::cout << "total_clause_size: " << end_size << std::endl;
            std::cout << "origin_clause_size: " << start_size << std::endl;
            std::cout << "origin_clause_num: " << clauses.size() << std::endl;
            std::cout << "learnt_clasue_num: " << learnts.size() << std::endl;
            std::cout << "current_prop " << total_prop << std::endl;
            std::cout << "totoal_real_time: " << duration_cast<seconds>(total_time_in_bcp).count() << std::endl;
            exit(0);
            //handle exit logic,
        }

        first_in = false;

        auto start = high_resolution_clock::now();
        CRef confl = propagate();
        auto end = high_resolution_clock::now();
        total_time_in_bcp += end - start;

        if (opt_warmup_prop > 0 and finished_init and not finished_warmup)
        {
            //std::cout<<"start warm up:"<<warmup_times<<std::endl;
            if (total_warmup == 0)
            {
                start_size = ca.size();
            }
            total_warmup++;
            //std::cout << "warmup:" << total_warmup << std::endl;
            if (total_warmup >= (unsigned long long)opt_warmup_prop)
            //if (warmup_times >= 100)
            {
                finished_warmup = true;
            }
        }
        if (finished_init)
            total_prop++;

        if (confl != CRef_Undef)
        {
            // CONFLICT
            conflicts++;
            curr_conflictC++;
            if (decisionLevel() == 0)
                return l_False;

            curr_learnt_clause.clear();
            analyze(confl, curr_learnt_clause, curr_backtrack_level);
            cancelUntil(curr_backtrack_level);

            if (curr_learnt_clause.size() == 1)
            {
                uncheckedEnqueue(curr_learnt_clause[0]);
            }
            else
            {
                CRef cr = ca.alloc(curr_learnt_clause, true);
                learnts.push(cr);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(curr_learnt_clause[0], cr);
            }

            varDecayActivity();
            claDecayActivity();

            if (--learntsize_adjust_cnt == 0)
            {
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt = (int)learntsize_adjust_confl;
                max_learnts *= learntsize_inc;

                if (verbosity >= 1)
                    printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n",
                           (int)conflicts,
                           (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals,
                           (int)max_learnts, nLearnts(), (double)learnts_literals / nLearnts(), progressEstimate() * 100);
            }
        }
        else
        {
            // NO CONFLICT
            if ((nof_conflicts >= 0 && curr_conflictC >= nof_conflicts) || !withinBudget())
            {
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef;
            }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            if (learnts.size() - nAssigns() >= max_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size())
            {
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True)
                {
                    // Dummy decision level:
                    newDecisionLevel();
                }
                else if (value(p) == l_False)
                {
                    analyzeFinal(~p, conflict);
                    return l_False;
                }
                else
                {
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef)
            {
                // New variable decision:
                decisions++;
                next = pickBranchLit();

                if (next == lit_Undef)
                    // Model found:
                    return l_True;
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}

double Solver::progressEstimate() const
{
    double progress = 0;
    double F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++)
    {
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...


 */

static double luby(double y, int x)
{

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x + 1; seq++, size = 2 * size + 1)
        ;

    while (size - 1 != x)
    {
        size = (size - 1) >> 1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    static bool first_in = true;
    if (!opt_load)
    {
        first_in = false;
    }
    if (!first_in)
    {
        model.clear();
        conflict.clear();
        if (!ok)
            return l_False;

        solves++;

        max_learnts = nClauses() * learntsize_factor;
        learntsize_adjust_confl = learntsize_adjust_start_confl;
        learntsize_adjust_cnt = (int)learntsize_adjust_confl;
        curr_stats = l_Undef;
    }
    if (verbosity >= 1)
    {
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    if (!first_in)
        curr_restarts = 0;
    while (curr_stats == l_Undef)
    {
        if (!first_in)
        {
            curr_rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
        }
        first_in = false;
        curr_stats = search(curr_rest_base * restart_first);
        if (!withinBudget())
            break;
        curr_restarts++;
    }

    if (verbosity >= 1)
        printf("===============================================================================\n");

    if (curr_stats == l_True)
    {
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++)
            model[i] = value(i);
    }
    else if (curr_stats == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);
    return curr_stats;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var> &map, Var &max)
{
    if (map.size() <= x || map[x] == -1)
    {
        map.growTo(x + 1, -1);
        map[x] = max++;
    }
    return map[x];
}

void Solver::toDimacs(FILE *f, Clause &c, vec<Var> &map, Var &max)
{
    if (satisfied(c))
        return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max) + 1);
    fprintf(f, "0\n");
}

void Solver::toDimacs(const char *file, const vec<Lit> &assumps)
{
    FILE *f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}

void Solver::toDimacs(FILE *f, const vec<Lit> &)
{
    // Handle case when solver is in contradictory state:
    if (!ok)
    {
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return;
    }

    vec<Var> map;
    Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
            cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
        {
            Clause &c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++)
    {
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max) + 1);
    }

    for (int i = 0; i < clauses.size(); i++)
        toDimacs(f, ca[clauses[i]], map, max);

    if (verbosity > 0)
        printf("Wrote %d clauses with %d variables.\n", cnt, max);
}

//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator &to)
{
    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++)
        {
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher> &ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++)
    {
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++)
        ca.reloc(learnts[i], to);

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++)
        ca.reloc(clauses[i], to);
}

void Solver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size() * ClauseAllocator::Unit_Size, to.size() * ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}
