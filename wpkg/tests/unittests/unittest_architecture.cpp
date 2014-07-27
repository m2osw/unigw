/*    unittest_architecture.cpp
 *    Copyright (C) 2014  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */

#include "unittest_architecture.h"
#include "unittest_main.h"
#include <stdexcept>
#include <cstring>



CPPUNIT_TEST_SUITE_REGISTRATION( ArchitectureUnitTests );


namespace
{

std::string replace_all(std::string& s, std::string const& to_replace, std::string const& replace_with)
{
    for(;;)
    {
        std::string::size_type pos(s.find(to_replace));
        if(pos == std::string::npos)
        {
            break;
        }
        s.replace(pos, to_replace.length(), replace_with);
    }
    return s;
}


} // no name namespace


void ArchitectureUnitTests::setUp()
{
}



void ArchitectureUnitTests::valid_vendors()
{
    // an empty name is always valid
    CPPUNIT_ASSERT(wpkg_architecture::architecture::valid_vendor(""));

    for(int c(1); c < 256; ++c)
    {
        std::string str("ven");
        str += c;
        str += "dor";
        if((c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '+'
        || c == '.')
        {
            CPPUNIT_ASSERT(wpkg_architecture::architecture::valid_vendor(str));
        }
        else
        {
            CPPUNIT_ASSERT(!wpkg_architecture::architecture::valid_vendor(str));
        }
    }
}


void ArchitectureUnitTests::verify_abbreviations()
{
    for(const wpkg_architecture::architecture::abbreviation_t *abbr(wpkg_architecture::architecture::abbreviation_list());; ++abbr)
    {
        if(abbr->f_abbreviation == nullptr)
        {
            return;
        }
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_abbreviation(abbr->f_abbreviation) == abbr);

        std::string str("invalid");
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_abbreviation(str + abbr->f_abbreviation) == nullptr);
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_abbreviation(abbr->f_abbreviation + str) == nullptr);
    }
}


void ArchitectureUnitTests::verify_os()
{
    for(const wpkg_architecture::architecture::os_t *os(wpkg_architecture::architecture::os_list());; ++os)
    {
        if(os->f_name == nullptr)
        {
            return;
        }
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_os(os->f_name) == os);

        if(strcmp(os->f_name, "mswindows") == 0)
        {
            CPPUNIT_ASSERT(wpkg_architecture::architecture::find_os("win32") == os);
            CPPUNIT_ASSERT(wpkg_architecture::architecture::find_os("win64") == os);
        }

        std::string str("invalid");
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_os(str + os->f_name) == nullptr);
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_os(os->f_name + str) == nullptr);
    }
}


void ArchitectureUnitTests::verify_processors()
{
    for(const wpkg_architecture::architecture::processor_t *processor(wpkg_architecture::architecture::processor_list());; ++processor)
    {
        if(processor->f_name == nullptr)
        {
            return;
        }
//std::cerr << "testing proc name = [" << processor->f_name << "]\n";
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(processor->f_name, false) == processor);

        std::string str("invalid");
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(str + processor->f_name, false) == nullptr);
        CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(str + processor->f_name, true) == nullptr);

        // the aliases vary dramatically depending on the processor and
        // we do not have a list of all possibilities but instead we use
        // patterns so we have to feed the system with specific entries
        // to test those...
        if(strcmp(processor->f_name, "alpha") == 0
        || strcmp(processor->f_name, "arm") == 0
        || strcmp(processor->f_name, "hppa") == 0)
        {
            for(int idx(0); idx <= 100; ++idx)
            {
                std::stringstream ss;
                ss << processor->f_name << idx;
                if(ss.str() != "arm64") // yeah... funny test!
                {
                    CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(ss.str(), false) != processor);
                    CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(ss.str(), true) == processor);
                }
            }
            {
                // but we forbid invalid (unwanted) characters such as '-'
                std::stringstream ss;
                ss << processor->f_name << "-";
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(ss.str(), false) != processor);
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(ss.str(), true) != processor);
            }
        }
        else
        {
            // "arm64invalid" and "armebinvalid" are accepted "arm" entries (yuck!)
            if(strcmp(processor->f_name, "arm64") != 0
            && strcmp(processor->f_name, "armeb") != 0)
            {
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(processor->f_name + str, false) == nullptr);
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(processor->f_name + str, true) == nullptr);
            }

            if(strcmp(processor->f_name, "arm64") == 0
            || strcmp(processor->f_name, "mips") == 0
            || strcmp(processor->f_name, "powerpc") == 0)
            {
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(processor->f_other_names, false) != processor);
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(processor->f_other_names, true) == processor);
            }
            else if(strcmp(processor->f_name, "armeb") == 0)
            {
                for(int idx(0); idx <= 100; ++idx)
                {
                    std::stringstream ss;
                    ss << "arm" << idx << "b";
                    CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(ss.str(), false) != processor);
                    CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor(ss.str(), true) == processor);
                }
                // but we forbid invalid (unwanted) characters such as '-'
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor("arm-b", false) != processor);
                CPPUNIT_ASSERT(wpkg_architecture::architecture::find_processor("arm-b", true) != processor);
            }
        }
    }
}


void ArchitectureUnitTests::verify_architecture()
{
    // empty architecture
    wpkg_architecture::architecture empty;
    CPPUNIT_ASSERT(empty.empty());
    CPPUNIT_ASSERT(!empty.is_pattern());
    CPPUNIT_ASSERT(!empty.is_source());
    CPPUNIT_ASSERT(!empty.is_unix());
    CPPUNIT_ASSERT(!empty.is_mswindows());
    CPPUNIT_ASSERT(empty.get_os().empty());
    CPPUNIT_ASSERT(empty.get_vendor().empty());
    CPPUNIT_ASSERT(empty.get_processor().empty());
    CPPUNIT_ASSERT(empty.to_string().empty());
    CPPUNIT_ASSERT(!empty.ignore_vendor());
    CPPUNIT_ASSERT(static_cast<std::string>(empty).empty());
    CPPUNIT_ASSERT(!static_cast<bool>(empty));
    CPPUNIT_ASSERT(!empty);

    empty.set_ignore_vendor(true);
    CPPUNIT_ASSERT(empty.ignore_vendor());
    empty.set_ignore_vendor(false);
    CPPUNIT_ASSERT(!empty.ignore_vendor());

    // test all combos, after all we do not really have any limits...
    {
        const char *vendors[] =
        {
            "any",
            "m2osw",
            "m2osw.com",
            "m2osw+3",
            "m2osw+3.1",
            "m2osw.com+31",
            nullptr
        };

        // Note: the loops of loops of loops... represent about
        //       ( 13 x 6 x 24 ) ** 2 so about 3,504,384 iterations

        for(const wpkg_architecture::architecture::os_t *os(wpkg_architecture::architecture::os_list()); os->f_name != nullptr; ++os)
        {
            std::cout << "." << std::flush;
            for(const char **v(vendors); *v != nullptr; ++v)
            {
                for(const wpkg_architecture::architecture::processor_t *processor(wpkg_architecture::architecture::processor_list()); processor->f_name != nullptr; ++processor)
                {
                    // No vendor
                    {
                        std::stringstream ss;
                        ss << os->f_name << "-" << processor->f_name;
                        wpkg_architecture::architecture arch(ss.str());

                        CPPUNIT_ASSERT(!arch.empty());
                        if(strcmp(os->f_name, "any") == 0
                        || strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(arch.is_pattern());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(!arch.is_pattern());
                        }
                        CPPUNIT_ASSERT(arch.is_source() ^ (arch.get_processor() != "source"));
                        if(arch.get_os() == "all"
                        || arch.get_os() == "any")
                        {
                            CPPUNIT_ASSERT(!arch.is_unix());
                            CPPUNIT_ASSERT(!arch.is_mswindows());
                        }
                        else if(arch.get_os() == "mswindows")
                        {
                            CPPUNIT_ASSERT(!arch.is_unix());
                            CPPUNIT_ASSERT(arch.is_mswindows());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(arch.is_unix());
                            CPPUNIT_ASSERT(!arch.is_mswindows());
                        }
                        CPPUNIT_ASSERT(arch.get_os() == os->f_name);
                        CPPUNIT_ASSERT(arch.get_vendor() == wpkg_architecture::architecture::UNKNOWN_VENDOR);
                        CPPUNIT_ASSERT(arch.get_processor() == processor->f_name);
                        CPPUNIT_ASSERT(!arch.ignore_vendor());
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(arch.to_string() == "any");
                            CPPUNIT_ASSERT(static_cast<std::string>(arch) == "any");
                        }
                        else
                        {
                            CPPUNIT_ASSERT(arch.to_string() == ss.str());
                            CPPUNIT_ASSERT(static_cast<std::string>(arch) == ss.str());
                        }
                        CPPUNIT_ASSERT(static_cast<bool>(arch));
                        CPPUNIT_ASSERT(!!arch);
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(arch == empty);
                        }
                        else
                        {
                            CPPUNIT_ASSERT(arch != empty);
                        }

                        // Test a copy of partial architecture
                        wpkg_architecture::architecture copy(arch);

                        CPPUNIT_ASSERT(!copy.empty());
                        if(strcmp(os->f_name, "any") == 0
                        || strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(copy.is_pattern());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(!copy.is_pattern());
                        }
                        CPPUNIT_ASSERT(copy.is_source() ^ (copy.get_processor() != "source"));
                        if(copy.get_os() == "all"
                        || copy.get_os() == "any")
                        {
                            CPPUNIT_ASSERT(!copy.is_unix());
                            CPPUNIT_ASSERT(!copy.is_mswindows());
                        }
                        else if(copy.get_os() == "mswindows")
                        {
                            CPPUNIT_ASSERT(!copy.is_unix());
                            CPPUNIT_ASSERT(copy.is_mswindows());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(copy.is_unix());
                            CPPUNIT_ASSERT(!copy.is_mswindows());
                        }
                        CPPUNIT_ASSERT(copy.get_os() == os->f_name);
                        CPPUNIT_ASSERT(copy.get_vendor() == wpkg_architecture::architecture::UNKNOWN_VENDOR);
                        CPPUNIT_ASSERT(copy.get_processor() == processor->f_name);
                        CPPUNIT_ASSERT(!copy.ignore_vendor());
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(copy.to_string() == "any");
                            CPPUNIT_ASSERT(static_cast<std::string>(copy) == "any");
                        }
                        else
                        {
                            CPPUNIT_ASSERT(copy.to_string() == ss.str());
                            CPPUNIT_ASSERT(static_cast<std::string>(copy) == ss.str());
                        }
                        CPPUNIT_ASSERT(static_cast<bool>(copy));
                        CPPUNIT_ASSERT(!!copy);
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(copy == empty);
                        }
                        else
                        {
                            CPPUNIT_ASSERT(copy != empty);
                        }

                        // we can compare with limits
                        CPPUNIT_ASSERT(arch == copy);
                        CPPUNIT_ASSERT(copy == arch);
                        CPPUNIT_ASSERT(!(arch != copy));
                        CPPUNIT_ASSERT(!(copy != arch));
                        CPPUNIT_ASSERT(!(arch < copy));
                        CPPUNIT_ASSERT(!(arch > copy));
                        CPPUNIT_ASSERT(!(copy < arch));
                        CPPUNIT_ASSERT(!(copy > arch));
                        CPPUNIT_ASSERT(arch <= copy);
                        CPPUNIT_ASSERT(arch >= copy);
                        CPPUNIT_ASSERT(copy <= arch);
                        CPPUNIT_ASSERT(copy >= arch);
                    }

                    // With Vendor
                    {
                        std::stringstream ss;
                        ss << os->f_name << "-" << *v << "-" << processor->f_name;
                        wpkg_architecture::architecture arch(ss.str());

                        CPPUNIT_ASSERT(!arch.empty());
                        if(strcmp(os->f_name, "any") == 0
                        || strcmp(*v, "any") == 0
                        || strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(arch.is_pattern());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(!arch.is_pattern());
                        }
                        CPPUNIT_ASSERT(arch.is_source() ^ (arch.get_processor() != "source"));
                        if(arch.get_os() == "all"
                        || arch.get_os() == "any")
                        {
                            CPPUNIT_ASSERT(!arch.is_unix());
                            CPPUNIT_ASSERT(!arch.is_mswindows());
                        }
                        else if(arch.get_os() == "mswindows")
                        {
                            CPPUNIT_ASSERT(!arch.is_unix());
                            CPPUNIT_ASSERT(arch.is_mswindows());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(arch.is_unix());
                            CPPUNIT_ASSERT(!arch.is_mswindows());
                        }
                        CPPUNIT_ASSERT(arch.get_os() == os->f_name);
                        CPPUNIT_ASSERT(arch.get_vendor() == *v);
                        CPPUNIT_ASSERT(arch.get_processor() == processor->f_name);
                        CPPUNIT_ASSERT(!arch.ignore_vendor());
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(*v, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(arch.to_string() == "any");
                            CPPUNIT_ASSERT(static_cast<std::string>(arch) == "any");
                        }
                        else
                        {
                            CPPUNIT_ASSERT(arch.to_string() == ss.str());
                            CPPUNIT_ASSERT(static_cast<std::string>(arch) == ss.str());
                        }
                        CPPUNIT_ASSERT(static_cast<bool>(arch));
                        CPPUNIT_ASSERT(!!arch);
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(*v, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(arch == empty);
                        }
                        else
                        {
                            CPPUNIT_ASSERT(arch != empty);
                        }

                        // Test copy of full arch.
                        wpkg_architecture::architecture copy(arch);

                        CPPUNIT_ASSERT(!copy.empty());
                        if(strcmp(os->f_name, "any") == 0
                        || strcmp(*v, "any") == 0
                        || strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(copy.is_pattern());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(!copy.is_pattern());
                        }
                        CPPUNIT_ASSERT(copy.is_source() ^ (copy.get_processor() != "source"));
                        if(copy.get_os() == "all"
                        || copy.get_os() == "any")
                        {
                            CPPUNIT_ASSERT(!copy.is_unix());
                            CPPUNIT_ASSERT(!copy.is_mswindows());
                        }
                        else if(copy.get_os() == "mswindows")
                        {
                            CPPUNIT_ASSERT(!copy.is_unix());
                            CPPUNIT_ASSERT(copy.is_mswindows());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(copy.is_unix());
                            CPPUNIT_ASSERT(!copy.is_mswindows());
                        }
                        CPPUNIT_ASSERT(copy.get_os() == os->f_name);
                        CPPUNIT_ASSERT(copy.get_vendor() == *v);
                        CPPUNIT_ASSERT(copy.get_processor() == processor->f_name);
                        CPPUNIT_ASSERT(!copy.ignore_vendor());
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(*v, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(copy.to_string() == "any");
                            CPPUNIT_ASSERT(static_cast<std::string>(copy) == "any");
                        }
                        else
                        {
                            CPPUNIT_ASSERT(copy.to_string() == ss.str());
                            CPPUNIT_ASSERT(static_cast<std::string>(copy) == ss.str());
                        }
                        CPPUNIT_ASSERT(static_cast<bool>(copy));
                        CPPUNIT_ASSERT(!!copy);
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(*v, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(copy == empty);
                        }
                        else
                        {
                            CPPUNIT_ASSERT(copy != empty);
                        }

                        // we can compare with limits
                        CPPUNIT_ASSERT(arch == copy);
                        CPPUNIT_ASSERT(copy == arch);
                        CPPUNIT_ASSERT(!(arch != copy));
                        CPPUNIT_ASSERT(!(copy != arch));
                        CPPUNIT_ASSERT(!(arch < copy));
                        CPPUNIT_ASSERT(!(arch > copy));
                        CPPUNIT_ASSERT(!(copy < arch));
                        CPPUNIT_ASSERT(!(copy > arch));
                        CPPUNIT_ASSERT(arch <= copy);
                        CPPUNIT_ASSERT(arch >= copy);
                        CPPUNIT_ASSERT(copy <= arch);
                        CPPUNIT_ASSERT(copy >= arch);

                        // Test the set() function directly
                        wpkg_architecture::architecture set;
                        set.set(arch);

                        CPPUNIT_ASSERT(!set.empty());
                        if(strcmp(os->f_name, "any") == 0
                        || strcmp(*v, "any") == 0
                        || strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(set.is_pattern());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(!set.is_pattern());
                        }
                        CPPUNIT_ASSERT(set.is_source() ^ (set.get_processor() != "source"));
                        if(set.get_os() == "all"
                        || set.get_os() == "any")
                        {
                            CPPUNIT_ASSERT(!set.is_unix());
                            CPPUNIT_ASSERT(!set.is_mswindows());
                        }
                        else if(set.get_os() == "mswindows")
                        {
                            CPPUNIT_ASSERT(!set.is_unix());
                            CPPUNIT_ASSERT(set.is_mswindows());
                        }
                        else
                        {
                            CPPUNIT_ASSERT(set.is_unix());
                            CPPUNIT_ASSERT(!set.is_mswindows());
                        }
                        CPPUNIT_ASSERT(set.get_os() == os->f_name);
                        CPPUNIT_ASSERT(set.get_vendor() == *v);
                        CPPUNIT_ASSERT(set.get_processor() == processor->f_name);
                        CPPUNIT_ASSERT(!set.ignore_vendor());
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(*v, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(set.to_string() == "any");
                            CPPUNIT_ASSERT(static_cast<std::string>(set) == "any");
                        }
                        else
                        {
                            CPPUNIT_ASSERT(set.to_string() == ss.str());
                            CPPUNIT_ASSERT(static_cast<std::string>(set) == ss.str());
                        }
                        CPPUNIT_ASSERT(static_cast<bool>(set));
                        CPPUNIT_ASSERT(!!set);
                        if(strcmp(os->f_name, "any") == 0
                        && strcmp(*v, "any") == 0
                        && strcmp(processor->f_name, "any") == 0)
                        {
                            CPPUNIT_ASSERT(set == empty);
                        }
                        else
                        {
                            CPPUNIT_ASSERT(set != empty);
                        }

                        // we can compare with limits
                        CPPUNIT_ASSERT(arch == set);
                        CPPUNIT_ASSERT(set == arch);
                        CPPUNIT_ASSERT(!(arch != set));
                        CPPUNIT_ASSERT(!(set != arch));
                        CPPUNIT_ASSERT(!(arch < set));
                        CPPUNIT_ASSERT(!(arch > set));
                        CPPUNIT_ASSERT(!(set < arch));
                        CPPUNIT_ASSERT(!(set > arch));
                        CPPUNIT_ASSERT(arch <= set);
                        CPPUNIT_ASSERT(arch >= set);
                        CPPUNIT_ASSERT(set <= arch);
                        CPPUNIT_ASSERT(set >= arch);

                        for(const wpkg_architecture::architecture::os_t *sub_os(wpkg_architecture::architecture::os_list()); sub_os->f_name != nullptr; ++sub_os)
                        {
                            for(const char **sub_v(vendors); *sub_v != nullptr; ++sub_v)
                            {
                                for(const wpkg_architecture::architecture::processor_t *sub_processor(wpkg_architecture::architecture::processor_list()); sub_processor->f_name != nullptr; ++sub_processor)
                                {
                                    std::stringstream sub_ss;
                                    sub_ss << sub_os->f_name << "-" << *sub_v << "-" << sub_processor->f_name;
                                    wpkg_architecture::architecture sub_arch(sub_ss.str());

//std::cerr << "testing " << ss.str() << " against " << sub_ss.str() << "\n";

                                    // do not ignore vendor
                                    bool const equal(ss.str() == sub_ss.str());
                                    if(arch.is_pattern() ^ sub_arch.is_pattern())
                                    {
                                        // compare between a pattern and a non-pattern
                                        bool pattern_equal(true);
                                        if(arch.is_pattern())
                                        {
                                            // arch is the pattern
                                            if(strcmp(os->f_name, "any") != 0)
                                            {
                                                pattern_equal = strcmp(os->f_name, sub_os->f_name) == 0;
                                            }
                                            if(pattern_equal && strcmp(*v, "any") != 0)
                                            {
                                                pattern_equal = strcmp(*v, *sub_v) == 0;
                                            }
                                            if(pattern_equal && strcmp(processor->f_name, "any") != 0)
                                            {
                                                pattern_equal = strcmp(processor->f_name, sub_processor->f_name) == 0;
                                            }
                                        }
                                        else
                                        {
                                            // sub_arch is the pattern
                                            if(strcmp(sub_os->f_name, "any") != 0)
                                            {
                                                pattern_equal = strcmp(os->f_name, sub_os->f_name) == 0;
                                            }
                                            if(pattern_equal && strcmp(*sub_v, "any") != 0)
                                            {
                                                pattern_equal = strcmp(*v, *sub_v) == 0;
                                            }
                                            if(pattern_equal && strcmp(sub_processor->f_name, "any") != 0)
                                            {
                                                pattern_equal = strcmp(processor->f_name, sub_processor->f_name) == 0;
                                            }
                                        }
                                        CPPUNIT_ASSERT((arch == sub_arch) ^ !pattern_equal);
                                        CPPUNIT_ASSERT((sub_arch == arch) ^ !pattern_equal);
                                        CPPUNIT_ASSERT((arch != sub_arch) ^ pattern_equal);
                                        CPPUNIT_ASSERT((sub_arch != arch) ^ pattern_equal);
                                    }
                                    else
                                    {
                                        // compare as is
                                        CPPUNIT_ASSERT((arch == sub_arch) ^ !equal);
                                        CPPUNIT_ASSERT((sub_arch == arch) ^ !equal);
                                        CPPUNIT_ASSERT((arch != sub_arch) ^ equal);
                                        CPPUNIT_ASSERT((sub_arch != arch) ^ equal);
                                    }

                                    std::string arch_str(ss.str());
                                    std::string sub_arch_str(sub_ss.str());
                                    replace_all(arch_str, "-", "\001");
                                    replace_all(sub_arch_str, "-", "\001");

                                    bool const less(arch_str < sub_arch_str);
//std::cerr << "converted for '<' [" << arch_str << "] < [" << sub_arch_str << "] -> " << less << "\n";
                                    CPPUNIT_ASSERT((arch < sub_arch) ^ !less);
                                    CPPUNIT_ASSERT((arch > sub_arch) ^ (less | equal));
                                    CPPUNIT_ASSERT((sub_arch < arch) ^ (less | equal));
                                    CPPUNIT_ASSERT((sub_arch > arch) ^ !less);

                                    CPPUNIT_ASSERT((arch <= sub_arch) ^ !(less | equal));
                                    CPPUNIT_ASSERT((arch >= sub_arch) ^ less);
                                    CPPUNIT_ASSERT((sub_arch <= arch) ^ less);
                                    CPPUNIT_ASSERT((sub_arch >= arch) ^ !(less | equal));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}




// vim: ts=4 sw=4 et
