#include "flowguard.hpp"

namespace fg = flowguard;


void fg::clean_defs_uses_map()
{
    // unordered_map<string, unordered_map<string, FGFunction*> >
    for (auto it = fg::g_def_uses->begin(); it != fg::g_def_uses->end(); ++it)
    {
        for (auto itt = it->second.begin(); itt != it->second.end(); ++itt)
        {
            delete itt->second;
        }
    }
    delete fg::g_def_uses;
}

void fg::clean_def_sets_map()
{
    delete fg::g_def_sets;
}

void fg::clean_var_set_map()
{
    delete fg::g_srcfunvar_setid;
}

char* fg::get_cstr_nameversion(tree *operand)
{
    const char *name = get_name(*operand);
    int version = SSA_NAME_VERSION(*operand);
    const char *s_version = to_string(version).c_str();
    int len_s_version = strlen(s_version);
    char *s_ver_under = new char[len_s_version + 2];
    s_ver_under[0] = '_';
    strncpy(s_ver_under + 1, s_version, len_s_version);
    s_ver_under[len_s_version + 1] = '\0';
    if (name)
    {
        int len_name = strlen(name);
        char *ret = new char[len_s_version + 1 + len_name + 1];
        strncpy(ret, name, len_name);
        strcpy(ret + len_name, s_ver_under);
        delete[] s_ver_under;
        return ret;
    }
    else
    {
        if (DECL_P(*operand) != 0 && TREE_CODE(*operand) == VAR_DECL)
        {
            name = to_string(DECL_UID(*operand)).c_str();
            int len_name = strlen(name);
            char *ret = new char[len_name + 1];
            strncpy(ret, name, len_name);
            ret[len_name] = '\0';
            return ret;
        }
    }
    return s_ver_under;
}

char* fg::get_cstr_namever_strict(tree *operand)
{
    /* TODO: Starting point of one of the compile-time errors.
     * It affects any variable stored in static storage (i.e., declared as 
     * static, globals, and extern globals). Specifically, it triggers when
     * retrieving the first operand, as it provides an invalid value that is
     * then propagated throughout the program. 
     * TODO v2: At first it looked like a problem with vars in static storage,
     * but it also crashes with other expressions. So, to take this path,
     * we also check the number of operands. */
    if (((TREE_CODE(*operand) == ADDR_EXPR || (TREE_CODE(*operand) == VAR_DECL
         && TREE_OPERAND_LENGTH(*operand) > 0))
         && POINTER_TYPE_P(TREE_TYPE(*operand))) ||
        (TREE_CODE(*operand) == MEM_REF)) // dereference operations
    {
        tree new_operand = TREE_OPERAND(*operand, 0);
        return fg::get_cstr_nameversion(&new_operand);
    }
    else
    {
        return fg::get_cstr_nameversion(operand);
    }
}

bool fg::is_string_constant(tree *var)
{
    bool ret = true;
    char *name = fg::get_cstr_namever_strict(var);
    if (strcmp(name, "_0") != 0)
        ret = false;
    delete[] name;
    return ret;
}


bool fg::FlowGuardSSACleanPass::gate(function *fun)
{
    return true;
}


unsigned int fg::FlowGuardSSACleanPass::execute(function *fun)
{
    const char *tmp_src = LOCATION_FILE(fun->function_start_locus);
    if (fg::len_current_src == 0)
    {
        fg::current_src = LOCATION_FILE(fun->function_start_locus);
        fg::len_current_src = strlen(fg::current_src);
    }
    else if (strcmp(fg::current_src, tmp_src) != 0)
    {
        fg::current_src = LOCATION_FILE(fun->function_start_locus);
        fg::len_current_src = strlen(fg::current_src);
    }
    fg::current_fun = function_name(fun);
    fg::len_current_fun = strlen(fg::current_fun);
    FGDefUse *defuse = new FGDefUse(fg::current_src, fg::current_fun,
                                    true, false);
    defuse->set_debug_defuses(true);
    FGAlias *alias = new FGAlias(fg::current_src, fg::current_fun);
    // Take note of all local decls and their unique uuid
    unsigned ii;
    tree local_decl;
    FOR_EACH_LOCAL_DECL(cfun, ii, local_decl)
    {
        char *decl_name = fg::get_cstr_nameversion(&local_decl);
        if (decl_name)
            alias->collect_uuid_ssaname(DECL_UID(local_decl), decl_name);
        delete[] decl_name;
    }
    int stmt_count = 1;
    basic_block bb;
    int bb_count = 2;
    // first recover phi information
    FOR_EACH_BB_FN(bb, fun)
    {
        for (gphi_iterator gphi_i = gsi_start_phis(bb); !gsi_end_p(gphi_i);
             gsi_next(&gphi_i))
        {
            gimple stmt = gsi_stmt(gphi_i);
            gphi *phi_stmt = as_a <gphi *>(stmt);
            tree result = gimple_phi_result(stmt);
            char *name_result = fg::get_cstr_namever_strict(&result);
            string name_result_str = string(name_result);
            defuse->get_phi_result_to_phi_arg()->emplace(string(name_result),
                                                         set<string>());
            int gen_id = defuse->generate_phi_result_id(name_result_str);
            (*defuse->get_phi_result_to_id())[name_result_str] = gen_id;
            for (unsigned int i = 0; i < gimple_phi_num_args(phi_stmt);
                 ++i)
            {
                tree arg = gimple_phi_arg_def(stmt, i);
                char *name_arg = fg::get_cstr_namever_strict(&arg);
                string name_arg_str = string(name_arg);
                if (defuse->get_phi_arg_to_phi_result()->count(
                        string(name_arg)) == 0)
                {
                    defuse->get_phi_arg_to_phi_result()->emplace(
                        string(name_arg), set<string>());
                }
                int loc = LOCATION_LINE(gimple_phi_arg_location(phi_stmt, i));
                (*defuse->get_phi_arg_to_phi_result())[
                    string(name_arg)].insert(string(name_result));
                (*defuse->get_phi_result_to_phi_arg())[
                    string(name_result)].insert(string(name_arg));
                delete[] name_arg;
            }
            delete[] name_result;
        }
        bb_count++;
    }
    bb_count = 2;
    FOR_EACH_BB_FN(bb, fun)
    {
        for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi);
             gsi_next(&gsi))
        {
            gimple stmt = gsi_stmt(gsi);
            int linenu = gimple_lineno(stmt);
            if (linenu == 0)
                continue;
            switch (gimple_code(stmt)) {
            case GIMPLE_ASSIGN:
            {
                tree lhs = gimple_assign_lhs(stmt);
                bool is_lhs_phi_arg = false;
                char *lhs_name = fg::get_cstr_namever_strict(&lhs);
                string lhs_name_str = string(lhs_name);
                delete[] lhs_name;
                if (!is_gimple_reg(lhs))
                {
                    defuse->add_def(lhs_name_str, linenu);
                }
                // Check if lhs of this statement is a phi argument in any
                // of the phis that we have recorded earlier.
                if (defuse->is_lhs_phi_arg(lhs_name_str))
                {
                    is_lhs_phi_arg = true;
                    if (defuse->get_phi_possible_ssa_defs()->count(
                            lhs_name_str) == 0)
                    {
                        defuse->get_phi_possible_ssa_defs()->emplace(
                            lhs_name_str, set<string>());
                    }
                }
                tree rhs1 = gimple_assign_rhs1(stmt);
                if (!is_gimple_constant(rhs1))
                {
                    if (!is_gimple_reg(rhs1))
                    {
                        char *name = fg::get_cstr_namever_strict(&rhs1);
                        defuse->add_use(string(name), linenu);
                        if (is_lhs_phi_arg)
                        {
                            (*defuse->get_phi_possible_ssa_defs())[
                                lhs_name_str].insert(string(name));
                        }
                        delete[] name;
                    }
                }
                tree rhs2 = gimple_assign_rhs2(stmt);
                if (rhs2)
                {
                    if (!is_gimple_constant(rhs2))
                    {
                        if (!is_gimple_reg(rhs2))
                        {
                            char *name = fg::get_cstr_namever_strict(&rhs2);
                            defuse->add_use(string(name), linenu);
                            if (is_lhs_phi_arg)
                            {
                                (*defuse->get_phi_possible_ssa_defs())[
                                    lhs_name_str].insert(string(name));
                            }
                            delete[] name;
                        }
                    }
                }
                break;
            }
            case GIMPLE_COND:
            {
                tree lhs = gimple_cond_lhs(stmt);
                tree rhs = gimple_cond_rhs(stmt);
                if (!is_gimple_constant(lhs))
                {
                    if (!is_gimple_reg(lhs))
                    {
                        char *name = fg::get_cstr_namever_strict(&lhs);
                        defuse->add_def(string(name), linenu);
                        delete[] name;
                    }
                }
                if (!is_gimple_constant(rhs))
                {
                     if (!is_gimple_reg(rhs))
                     {
                         char *name = fg::get_cstr_namever_strict(&rhs);
                         defuse->add_use(string(name), linenu);
                         delete[] name;
                     }
                }
                break;
            }
            case GIMPLE_CALL:
            {
                tree lhs = gimple_call_lhs(stmt);
                if (lhs)
                {
                    if (!is_gimple_reg(lhs))
                    {
                        char *name = fg::get_cstr_namever_strict(&lhs);
                        defuse->add_def(string(name), linenu);
                        delete[] name;
                    }
                }
                for (unsigned int i = 0; i < gimple_call_num_args(stmt); i++)
                {
                    tree *arg = gimple_call_arg_ptr(stmt, i);
                    if (!is_gimple_constant(*arg))
                    {
                        if (!is_gimple_reg(*arg))
                        {
                            char *name = fg::get_cstr_namever_strict(arg);
                            // some string constants are added as _0, we don't
                            // need them
                            if (strcmp(name, "_0") != 0)
                                defuse->add_use(string(name), linenu);
                            delete[] name;
                        }
                    }
                }
                break;
            }
            case GIMPLE_RETURN:
            {
                greturn *gr = as_a<greturn *>(stmt);
                tree ret = gimple_return_retval(gr);
                if (ret && !is_gimple_constant(ret))
                {
                    if (!is_gimple_reg(ret))
                    {
                        char *name = fg::get_cstr_namever_strict(&ret);
                        defuse->add_use(string(name), linenu);
                        delete[] name;
                    }
                }
                break;
            }
            case GIMPLE_SWITCH:
            {
                gswitch *gs = as_a<gswitch*>(stmt);
                tree index = gimple_switch_index(gs);
                if (!is_gimple_constant(index))
                {
                    if (!is_gimple_reg(index))
                    {
                        char *name = fg::get_cstr_namever_strict(&index);
                        defuse->add_use(string(name), linenu);
                        delete[] name;
                    }
                }
                break;
            }
            }
        }
    }
    if (fg::g_def_uses->count(defuse->get_source_file()) == 0)
    {
        fg::g_def_uses->emplace(make_pair(defuse->get_source_file(),
                                          unordered_map<string, FGDefUse*>()));
    }
    (*fg::g_def_uses)[defuse->get_source_file()].emplace(
        make_pair(defuse->get_func_name(), defuse));
    compute_may_aliases();
    delete alias;
    return 0;
}


bool fg::FlowGuardInstruPass::gate(function *fun)
{
    string func(function_name(fun));
    size_t pos = func.find("int main()");
    if (pos == string::npos)
        pos = func.find("int main(int, char**)");
    /* TODO: In C programs function_name() only returns the name (e.g., main)
     * so check again for it */
    if (pos == string::npos) {
        if (func.compare("main") == 0)
            pos = 1;
    }
    this->instrument_init = (pos != string::npos);
    return true;
}

unsigned int fg::FlowGuardInstruPass::execute(function *fun)
{
    this->instru = new FGInstru(g_def_uses, g_def_sets, g_srcfunvar_setid,
                                true);
    this->set_current_defuse((*g_def_uses)[fg::current_src][fg::current_fun]);
    basic_block bb, bb_2;
    int bb_count = 2;
    FOR_EACH_BB_FN(bb, fun)
    {
        if (bb_count == 2)
            bb_2 = bb;
        for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi);
             gsi_next(&gsi))
        {
            gimple stmt = gsi_stmt(gsi);
            if (bb_count == 2)
            {
                if (this->instrument_init)
                {
                    gimple_stmt_iterator tmp_gsi = gsi_start_bb(bb);
                    this->inject_initialization(&tmp_gsi);
                    this->instrument_init = false;
                }
            }
            int linenu = gimple_lineno(stmt);
            if (linenu == 0)
                continue;
            switch (gimple_code(stmt)) {
            case GIMPLE_ASSIGN:
            {
                tree lhs = gimple_assign_lhs(stmt);
                if (!is_gimple_reg(lhs)) {
                    /* TODO: Error triggered for structures that contain a
                     * bit-field that immediately follows another bit-field.
                     * Bitfields can overlap at RTL level */
                    this->set_def(&gsi, &lhs);
                }
                char *lhs_name = fg::get_cstr_namever_strict(&lhs);
                string lhs_name_str = string(lhs_name);
                delete[] lhs_name;
                tree rhs1 = gimple_assign_rhs1(stmt);

                if (!is_gimple_constant(rhs1) && !fg::is_string_constant(&rhs1)
                    && !is_gimple_reg(rhs1))
                {
                    bool phi = false;
                    char *rhs1_name = fg::get_cstr_namever_strict(&rhs1);
                    if (this->current_defuse->is_rhs_phi_result(
                            string(rhs1_name)))
                    {
                        this->check_phi(&gsi, &rhs1);
                        phi = true;
                    }
                    delete[] rhs1_name;
                    if (this->current_defuse->is_lhs_phi_arg(lhs_name_str))
                    {
                        this->set_phi(&gsi, &rhs1, lhs_name_str);
                        phi = true;
                    }
                    if (!phi)
                        this->check_def(&gsi, &rhs1);
                }
                tree rhs2 = gimple_assign_rhs2(stmt);
                if (rhs2 && !is_gimple_constant(rhs2)
                    && !fg::is_string_constant(&rhs2) && !is_gimple_reg(rhs2))
                {
                    this->check_def(&gsi, &rhs2);
                }
                break;
            }
            case GIMPLE_COND:
            {
                tree lhs = gimple_cond_lhs(stmt);
                if (!is_gimple_constant(lhs) && !fg::is_string_constant(&lhs)
                    && !is_gimple_reg(lhs))
                {
                    this->check_def(&gsi, &lhs);
                }
                tree rhs = gimple_cond_rhs(stmt);
                if (!is_gimple_constant(rhs) && !fg::is_string_constant(&rhs)
                    && !is_gimple_reg(rhs))
                {
                    this->check_def(&gsi, &rhs);
                }
                break;
            }
            case GIMPLE_CALL:
            {
                tree lhs = gimple_call_lhs(stmt);
                if (lhs && !fg::is_string_constant(&lhs)
                    && !is_gimple_reg(lhs))
                {
                    this->set_def(&gsi, &lhs);
                }
                for (unsigned int i = 0; i < gimple_call_num_args(stmt); i++)
                {
                    tree *arg = gimple_call_arg_ptr(stmt, i);
                    if (!is_gimple_constant(*arg)
                        && !fg::is_string_constant(arg)
                        && !is_gimple_reg(*arg))
                    {
                        this->check_def(&gsi, arg);
                    }
                }
                break;
            }
            case GIMPLE_RETURN:
            {
                greturn *gr = as_a<greturn*>(stmt);
                tree ret = gimple_return_retval(gr);
                if (ret && !is_gimple_constant(ret)
                    && !fg::is_string_constant(&ret)
                    && !is_gimple_reg(ret))
                {
                    this->check_def(&gsi, &ret);
                }
                break;
            }
            case GIMPLE_SWITCH:
            {
                gswitch *gs = as_a<gswitch*>(stmt);
                tree index = gimple_switch_index(gs);
                if (!is_gimple_constant(index)
                    && !fg::is_string_constant(&index)
                    && !is_gimple_reg(index))
                {
                    this->check_def(&gsi, &index);
                }
                break;
            }
            }
        }
        for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi);
             gsi_next(&gsi))
        {
            gimple stmt = gsi_stmt(gsi);
            switch (gimple_code(stmt)) {
            case GIMPLE_CALL:
            {
                break;
            }
            }
        }
#if fg_debug
        printf("\n->[ modified BB %d\n", bb_count);
        printf("->[\n");
        dump_bb(stdout, bb, 5, TDF_VOPS);
        printf("\n");
#endif
        bb_count++;
    }
    delete this->instru;
}

void fg::FlowGuardInstruPass::set_def(gimple_stmt_iterator *gsi, tree *def)
{
    char *name = fg::get_cstr_namever_strict(def);
    if (this->get_current_defuse()->has_strict_definitions(string(name)))
        this->inject_set_def(gsi, def, name);
    delete[] name;
}

void fg::FlowGuardInstruPass::check_def(gimple_stmt_iterator *gsi, tree *use)
{
    char *name = fg::get_cstr_namever_strict(use);
    if (this->get_current_defuse()->has_strict_definitions(string(name)))
        this->inject_check_def(gsi, use, string(name));
    delete[] name;
}


void fg::FlowGuardInstruPass::set_phi(gimple_stmt_iterator *gsi, tree *rhs,
                                      string phi_name)
{
    // we are given a phi arg, get phi result for this arg
    if (((*this->current_defuse->get_phi_arg_to_phi_result())[
             phi_name]).size() <= 1)
    {
        string phi_result = *(
            (*this->current_defuse->get_phi_arg_to_phi_result())[
                phi_name]).begin();
        this->inject_set_phi_to_var(gsi, rhs, phi_result); 
    }
}

void fg::FlowGuardInstruPass::check_phi(gimple_stmt_iterator *gsi, tree *rhs)
{
    char *phi_name = fg::get_cstr_namever_strict(rhs);
    string phi_name_str = string(phi_name);
    delete[] phi_name;
    this->inject_check_phi(gsi, rhs, phi_name_str);
}

void
fg::FlowGuardInstruPass::inject_set_def(gimple_stmt_iterator *gsi, tree *def,
                                        string name)
{
    gimple stmt = gsi_stmt(*gsi);
    tree address_of;

    /* TODO: We add the same trick that ASAN, UBSAN and TSAN use with
     * bit-fields */
    if (TREE_CODE(*def) == COMPONENT_REF &&
            DECL_BIT_FIELD_REPRESENTATIVE(TREE_OPERAND(*def, 1)) != NULL_TREE)
    {
//        tree repr = DECL_BIT_FIELD_REPRESENTATIVE(TREE_OPERAND(*def, 1));
//        address_of = build_addr(build3 (COMPONENT_REF, TREE_TYPE(repr),
//                    TREE_OPERAND(*def, 0), repr, NULL_TREE),
//                    current_function_decl);
        return;
    }
    else
    {
        address_of = build_addr(*def, current_function_decl);
    }

    /* TODO: Add the address_of tree to the set of processed exprs */
    this->current_defuse->get_processed_exprs()->insert(address_of);

    tree args = build_function_type_list(void_type_node, ptr_type_node,
                                         integer_type_node, NULL_TREE);
    int linenu = gimple_lineno(stmt);
    tree fnc_decl = build_fn_decl("_Z15fg_set_def_addrPvi", args);
    gimple gcall = gimple_build_call(fnc_decl, 2, address_of,
                                     build_int_cst(integer_type_node, linenu));
    gsi_insert_before(gsi, gcall, GSI_SAME_STMT);
}

void
fg::FlowGuardInstruPass::inject_check_def(gimple_stmt_iterator *gsi, tree *use,
                                          string name)
{
    int set_id = this->instru->get_setid_for_var(name.c_str(), fg::current_fun,
                                                 fg::current_src);
    gimple stmt = gsi_stmt(*gsi);
    tree address_of;
    if (POINTER_TYPE_P(TREE_TYPE(*use)) && TREE_CODE(*use) == ADDR_EXPR)
    {
        address_of = *use;
    }
    /* TODO: Add the same trick as in set_def for bit-field expressions */
    else if (TREE_CODE(*use) == COMPONENT_REF &&
            DECL_BIT_FIELD_REPRESENTATIVE(TREE_OPERAND(*use, 1)) != NULL_TREE)
    {
//        tree repr = DECL_BIT_FIELD_REPRESENTATIVE(TREE_OPERAND(*use, 1));
//        address_of = build_addr(build3 (COMPONENT_REF, TREE_TYPE(repr),
//                    TREE_OPERAND(*use, 0), repr, NULL_TREE),
//                    current_function_decl);
          return;
    }
    else
    {
        address_of = build_addr(*use, current_function_decl);
    }

    /* TODO: Check if we have inserted a call to set_def before */
    if (this->current_defuse->get_processed_exprs()->count(address_of) != 0) {
        this->current_defuse->get_processed_exprs()->erase(address_of);
    } else {
        return;
    }

    tree args = build_function_type_list(void_type_node, ptr_type_node,
                                         integer_type_node, NULL_TREE);
    tree fnc_decl = build_fn_decl("_Z17fg_check_def_addrPvi", args);
    gimple gcall = gimple_build_call(fnc_decl, 2, address_of,
                                     build_int_cst(integer_type_node, set_id));
    gsi_insert_before(gsi, gcall, GSI_SAME_STMT);
}

void
fg::FlowGuardInstruPass::inject_set_phi_to_var(gimple_stmt_iterator *gsi,
                                               tree *rhs, string phi_name)
{
    if (this->current_defuse->get_phi_result_to_id()->count(phi_name) != 0)
    {
        int phi_id = (*this->current_defuse->get_phi_result_to_id())[phi_name];
        /* TODO: Add the phi id to the set of processed phis */
        this->current_defuse->get_processed_phis()->insert(phi_id);

        char *rhs_name = fg::get_cstr_namever_strict(rhs);
        int set_id = this->instru->get_setid_for_var(rhs_name, fg::current_fun,
                                                     fg::current_src);
        delete[] rhs_name;
        tree address_of;
        if (POINTER_TYPE_P(TREE_TYPE(*rhs)) && TREE_CODE(*rhs) == ADDR_EXPR)
            address_of = *rhs;
        else
            address_of = build_addr(*rhs, current_function_decl);
        tree args = build_function_type_list(void_type_node, integer_type_node,
                                             integer_type_node, ptr_type_node,
                                             NULL_TREE);
        tree fnc_decl = build_fn_decl("_Z17fg_set_phi_to_variiPv", args);
        gimple gcall = gimple_build_call(
            fnc_decl, 3, build_int_cst(integer_type_node, phi_id),
            build_int_cst(integer_type_node, set_id), address_of);
        gsi_insert_before(gsi, gcall, GSI_SAME_STMT);
    }
}

void
fg::FlowGuardInstruPass::inject_check_phi(gimple_stmt_iterator *gsi, tree *rhs,
                                          string phi_name)
{
    if (this->current_defuse->get_phi_result_to_id()->count(phi_name) != 0)
    {
        int phi_id = (*this->current_defuse->get_phi_result_to_id())[phi_name];
        
        /* TODO: Check if we have inserted a call to set_phi before */
        if (this->current_defuse->get_processed_phis()->count(phi_id) != 0) {
            this->current_defuse->get_processed_phis()->erase(phi_id);
        } else {
            return;
        }

        tree address_of;
        if (POINTER_TYPE_P(TREE_TYPE(*rhs)) && TREE_CODE(*rhs) == ADDR_EXPR)
        {
            address_of = *rhs;
        }
        else
        {
            if (TREE_CODE(*rhs) == MEM_REF)
            {
                address_of = build_addr(*rhs, current_function_decl);
                /* TODO: When the following if condition is met, build_addr is
                 * called twice. This function call takes the address of the
                 * expression arg0 to be used within the context arg1.
                 * As a result, the built expression takes the following form:
                 * &&MEM[...], which I guess is invalid due to multiple
                 * consecutive "&" operators.
                 */
//                if (!is_gimple_reg(address_of))
//                {
//                    address_of = build_addr(address_of, current_function_decl);
//                }
            }
            else
            {
                address_of = build_addr(*rhs, current_function_decl);
            }
        }
        if (!is_gimple_reg(address_of))
        {
            tree args = build_function_type_list(
                void_type_node, integer_type_node, ptr_type_node, NULL_TREE);
            tree fnc_decl = build_fn_decl("_Z12fg_check_phiiPv", args);
            gimple gcall = gimple_build_call(
                fnc_decl, 2, build_int_cst(integer_type_node, phi_id),
                address_of);
            gsi_insert_before(gsi, gcall, GSI_SAME_STMT);
        }
    }
}

void fg::FlowGuardInstruPass::inject_initialization(gimple_stmt_iterator *gsi)
{
     tree args = build_function_type_list(void_type_node, NULL_TREE);
     tree decl = build_fn_decl("_Z15fg_load_staticsv", args);
     gimple gcall = gimple_build_call(decl, 0);
     gsi_insert_before(gsi, gcall, GSI_SAME_STMT);
}

void dump_dfg()
{
    fg::write_dfg(fg::g_srcfunvar_setid, fg::g_def_sets, fg::dfg_path);
}

void plugin_end_2(void *gcc_data, void *user_data)
{
    dump_dfg();
    fg::clean_var_set_map();
    fg::clean_def_sets_map();
    fg::clean_defs_uses_map();
}

/* Initialize set counter */
int fg::FGInstru::set_id = 1;

int
plugin_init(struct plugin_name_args* plugin_info,
            struct plugin_gcc_version* version)
{
    if (!plugin_default_version_check(version, &gcc_version))
    {
        printf(" This GCC plugin is for version %d.%d\n",
               GCCPLUGIN_VERSION_MAJOR, GCCPLUGIN_VERSION_MINOR);
        return 1;
    }
    bool update_setid = false;
    // Check if we have received plugin args (dfgpath: tells us where we have
    // to produce the dfg
    if (plugin_info->argc > 0)
    {
        for (int i = 0; i < plugin_info->argc; ++i)
        {
            if (strcmp(plugin_info->argv[i].key, "dfgpath") == 0)
            {
                fg::dfg_path = plugin_info->argv[i].value;
                update_setid = true;
            }
        }
    }
    // Check if we need to update the set ids since we are compiling more than
    // one source
    if (update_setid)
    {
        string path(fg::dfg_path);
        path += "/fg_dfg_sets.fg";
        int highest_set = fg::get_highest_set(path);
        if (highest_set == -1)
        {
            error("the static DFG sets file is corrupt, highest set is -1");
        }
        else
        {
            // set the set id to be the next
            fg::FGInstru::set_id = highest_set++;
        }
    }
    // initialise structures
    fg::g_def_uses = new unordered_map<
        string, unordered_map<string, fg::FGDefUse*> >();
    fg::g_def_sets = new unordered_map<int, set<int> >();
    fg::g_srcfunvar_setid = new unordered_map<string, int>();
    // attach passes
    struct register_pass_info fgssa_pass_info;
    fgssa_pass_info.pass = new fg::FlowGuardSSACleanPass(g);
    fgssa_pass_info.reference_pass_name = "ssa";
    fgssa_pass_info.ref_pass_instance_number = 1;
    fgssa_pass_info.pos_op = PASS_POS_INSERT_AFTER;
    register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP,
                      NULL, &fgssa_pass_info);
    struct register_pass_info fginstru_pass_info;
    fginstru_pass_info.pass = new fg::FlowGuardInstruPass(g);
    fginstru_pass_info.reference_pass_name = "flowguard_ssa_clean";
    fginstru_pass_info.ref_pass_instance_number = 1;
    fginstru_pass_info.pos_op = PASS_POS_INSERT_AFTER;
    register_callback(plugin_info->base_name, PLUGIN_PASS_MANAGER_SETUP,
                      NULL, &fginstru_pass_info);
    register_callback(plugin_info->base_name, PLUGIN_FINISH, &plugin_end_2,
                      NULL);
    return 0;
}
