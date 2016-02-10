/***********************************************************
 * (c) Kancelaria Prezesa Rady Ministrów 2012-2015         *
 * Treść licencji w pliku 'LICENCE'                        *
 *                                                         *
 * (c) Chancellery of the Prime Minister 2012-2015         *
 * License terms can be found in the file 'LICENCE'        *
 *                                                         *
 * Author: Grzegorz Klima                                  *
 ***********************************************************/

/** \file model_parse.cpp
 * \brief Parsing model.
 */

#include <model_parse.h>
#include <stdexcept>
#include <vector>
#include <string>
#ifdef R_DLL
#include <R.h>
#include <Rcpp.h>
#endif /* R_DLL */
#include <utils.h>
#include <gecon_info.h>
#include <gecon_tokens.h>
#include "gEconLexer.hpp"
#include "gEconParser.hpp"

unsigned char **tnames;

using namespace parser;

std::vector<std::string> errors;
Model model_obj;


void
report_errors(const std::string &mes)
{
#ifdef R_DLL
    Rf_error(('\n' + mes).c_str());
#else /* R_DLL */
    std::cerr << "errors:\n" << mes << '\n';
    exit(1);
#endif /* R_DLL */
}


void
report_warns(const std::string &mes)
{
#ifdef R_DLL
    warning(('\n' + mes).c_str());
#else /* R_DLL */
    std::cerr << "warnings:\n" << mes << '\n';
#endif /* R_DLL */
}


void
write_info(const std::string &mes)
{
#ifdef R_DLL
    Rprintf((mes + '\n').c_str());
#else /* R_DLL */
    std::cerr << mes << '\n';
#endif /* R_DLL */
}


#ifdef R_DLL
RcppExport
SEXP
parse_from_R(SEXP inputname)
{
    std::string name = Rcpp::as<std::string>(inputname);
    model_parse(name.c_str());
    return Rcpp::wrap(true);
}
#endif /* R_DLL */



std::string
get_mod_name(const std::string &s)
{
    unsigned l = s.size();
    if ((l < 5) || (s[l-1] != 'n') || (s[l-2] != 'c')
        || (s[l-3] != 'g') || (s[l-4] != '.')) return std::string();
    std::string ret = s;
    ret.erase(l - 4, l - 1);
    return ret;
}


void
model_parse(const char *fname)
{
    unsigned char **tnames = mk_tnames();
    model_obj.clear();
    std::string name(fname), mod_name;

    write_info(gecon_hello_str());
    mod_name = get_mod_name(name);
    if (!mod_name.size()) {
        report_errors("(gEcon error): invalid model file name or extension: \'" + name + "\'");
    }
    model_obj.set_name(mod_name);

    ANTLR_UINT8 *fName;
    fName = (ANTLR_UINT8*) fname;

    try {
        gEconLexer::InputStreamType input(fName, ANTLR_ENC_8BIT);
        gEconLexer lxr(&input);   // CLexerNew is generated by ANTLR
        gEconParser::TokenStreamType tstream(ANTLR_SIZE_HINT, lxr.get_tokSource() );
        gEconParser psr(&tstream);   // CParserNew is generated by ANTLR3
        psr.get_state()->set_tokenNames(tnames);
        psr.model();
        free_tnames(tnames);
    }
    catch (std::bad_alloc &ba)
    {
        errors.clear();
        model_obj.clear();
        free_tnames(tnames);
        report_errors(std::string("(gEcon error): out of memory"));
    }
    catch (antlr3::ParseFileAbsentException &pfa)
    {
        errors.clear();
        model_obj.clear();
        free_tnames(tnames);
        report_errors(std::string("(gEcon error): cannot open file \'") +
                      name + "\'");
    }
    catch (std::exception &e) {
        errors.clear();
        model_obj.clear();
        free_tnames(tnames);
#ifdef R_DLL
        report_errors(std::string("(gEcon internal error): this is a bug :-(, please \
report it (with the .gcn file that caused this message) to " + gecon_bug_str()));
#else /* R_DLL */
        report_errors(std::string("(gEcon internal error): ") + e.what());
#endif /* R_DLL */
    }

    if (errors.size()) {
        std::string mes;
        int i, n = errors.size();
        for (i = 0; i < n; ++i)
            mes += "(gEcon parse error " + symbolic::internal::num2str(i + 1)
                   + "): " + errors[i] + '\n';
        errors.clear();
        model_obj.clear();
        report_errors(mes);
    }

    try {
        model_obj.do_it();
    }
    catch (std::bad_alloc &ba)
    {
        errors.clear();
        model_obj.clear();
        report_errors(std::string("(gEcon error): out of memory"));
    }
    catch (std::runtime_error &e) {
        errors.clear();
        model_obj.clear();
#ifdef R_DLL
        report_errors(std::string("(gEcon internal error): this is a bug :-(, please \
report it (with the .gcn file that caused this message) to " + gecon_bug_str()));
#else /* R_DLL */
        report_errors(std::string("(gEcon internal error): ") + e.what());
#endif /* R_DLL */
    }
    if (model_obj.warnings()) {
        std::string mes(model_obj.get_warns());
        report_warns(mes);
    }
    if (model_obj.errors()) {
        std::string mes(model_obj.get_errs());
        model_obj.clear();
        report_errors(mes);
    }

    model_obj.write();
    model_obj.clear();
}




















