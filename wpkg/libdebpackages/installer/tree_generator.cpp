/*    installer/tree_generator.cpp
 *    Copyright (C) 2012-2015  Made to Order Software Corporation
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
 *    Doug Barbieri  doug@m2osw.com
 */

/** \class tree_generator
 * \brief Generate all possible permutations of the package tree.
 *
 * Lazily generates all possible permutations of the package tree, such
 * that only one version of any named package is installable. The resulting
 * permutations are not guaranteed to be valid, checking the validity is
 * done afterward.
 *
 * Uses the cartesian product algorithm described here:
 *   http://phrogz.net/lazy-cartesian-product
 *
 * \todo
 * Hack out the package_item_t::list_t-specific lazy cartesian product
 * generator and turn it into a generic one.
 */

#include "libdebpackages/installer/tree_generator.h"

#include <set>


namespace wpkgar
{

namespace installer
{

/** \brief Initialize a tree generator object.
 *
 * This function pre-computes indices to make generating the
 * cartesian product of the package options a bit easier
 * later on.
 *
 * \attention
 * The behaviour is undefined if the order of the packages in the
 * master tree is changed while the tree_generator exists.
 *
 * \param[in] root_tree An immutable reference to the master package tree that
 *                      will serve as the source of the tree permutations.
 */
tree_generator::tree_generator(const package_item_t::list_t& root_tree)
    : f_master_tree(root_tree)
    //, f_pkg_alternatives() -- auto-init
    //, f_divisor() -- auto-init
    //, f_n(0) -- auto-init
    //, f_end(0) -- auto-init
{
    std::set<std::string> visited_packages;

    // Pre-compute the alternatives lists that we can then simply walk over
    // to generate the various permutations of the tree later on.
    for(package_index_t item_idx(0); item_idx < f_master_tree.size(); ++item_idx)
    {
        const package_item_t& pkg(f_master_tree[item_idx]);
        const std::string pkg_name(pkg.get_name());

        // have we already dealt with all the packages by this name?
        if(!visited_packages.insert(pkg_name).second)
        {
            continue;
        }

        pkg_alternatives_t alternatives;
        if(pkg.get_type() == package_item_t::package_type_available)
        {
            alternatives.push_back(item_idx);
        }

        for(package_index_t candidate_idx(0); candidate_idx < f_master_tree.size(); ++candidate_idx)
        {
            if(item_idx == candidate_idx)
            {
                // ignore self
                continue;
            }

            const package_item_t& candidate(f_master_tree[candidate_idx]);
            if(candidate.get_type() != package_item_t::package_type_available)
            {
                continue;
            }

            const std::string candidate_name(candidate.get_name());
            if(pkg_name == candidate_name)
            {
                alternatives.push_back(candidate_idx);
            }
        }

        if(!alternatives.empty())
        {
            f_pkg_alternatives.push_back(alternatives);
        }
    }

    // Pre-compute the divisors that we need to walk the list-of-alternatives-
    // lists such that we end up with the cartesian product of all the
    // alternatives.
    package_index_t factor(1);
    f_divisor.resize(f_pkg_alternatives.size());
    pkg_alternatives_list_t::size_type i(f_pkg_alternatives.size());
    while(i > 0)
    {
        --i;
        f_divisor[static_cast<package_index_t>(i)] = factor;
        factor = f_pkg_alternatives[static_cast<package_index_t>(i)].size() * factor;
    }
    f_end = factor;
}


/** \brief Computer the next permutation.
 *
 * This function computes and returns the next permutation of the master
 * package tree using the data generated in the constructor.
 *
 * \returns Returns a package_item_t::list_t where exactly one version of any
 *          given package is enabled. Returns an empty package_item_t::list_t
 *          when all possibilities have been exhausted.
 */
package_item_t::list_t tree_generator::next()
{
    package_item_t::list_t result;

    if(f_n < f_end)
    {
        result = f_master_tree;

        // for each group of version-specific alternatives ...
        for(package_index_t pkg_set(0); pkg_set < f_pkg_alternatives.size(); ++pkg_set)
        {
            // select one alternative from the list ...
            const pkg_alternatives_t& options(f_pkg_alternatives[pkg_set]);
            const package_index_t target_idx((f_n / f_divisor[pkg_set]) % options.size());

            // ... and mark the rest as invalid.
            for(package_index_t option_idx(0); option_idx < options.size(); ++option_idx)
            {
                if(option_idx != target_idx)
                {
                    // all except self
                   result[options[option_idx]].set_type(package_item_t::package_type_invalid);
                }
            }
        }

        f_n = f_n + 1;
    }

    return result;
}


/** \brief Return the current tree number.
 *
 * This function returns the number of the last tree returned by the
 * next() function. If the next() function was never called, then
 * the function returns zero. It can also return zero if no tree can
 * be generated.
 *
 * \warning
 * This means the tree number is 1 based which in C++ is uncommon!
 *
 * \return The current tree.
 */
uint64_t tree_generator::tree_number() const
{
    return f_n;
}

}
// namespace installer

}
// namespace wpkgar

// vim: ts=4 sw=4 et
