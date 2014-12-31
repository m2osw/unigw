/*    wpkg_field.h -- declaration of the field file format
 *    Copyright (C) 2012-2014  Made to Order Software Corporation
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

/** \file
 * \brief Base class used to read files of fields.
 *
 * This class defines the base functions used to read and manage a file
 * composed of fields. This is used for control, control.info, copyright,
 * and pkgconfig (.pc) files.
 */
#ifndef WPKG_FIELD_H
#define WPKG_FIELD_H
#include    "memfile.h"
#include    "case_insensitive_string.h"
#include    "wpkg_output.h"


namespace wpkg_field
{

class wpkg_field_exception : public std::runtime_error
{
public:
    wpkg_field_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkg_field_exception_invalid : public wpkg_field_exception
{
public:
    wpkg_field_exception_invalid(const std::string& what_msg) : wpkg_field_exception(what_msg) {}
};

class wpkg_field_exception_cyclic : public wpkg_field_exception
{
public:
    wpkg_field_exception_cyclic(const std::string& what_msg) : wpkg_field_exception(what_msg) {}
};

class wpkg_field_exception_undefined : public wpkg_field_exception
{
public:
    wpkg_field_exception_undefined(const std::string& what_msg) : wpkg_field_exception(what_msg) {}
};






class DEBIAN_PACKAGE_EXPORT field_file
{
public:
    typedef std::vector<std::string>    list_t;

    class DEBIAN_PACKAGE_EXPORT field_t
    {
    public:
                        field_t();
                        field_t(const field_t& rhs);
                        field_t(const field_file& file, const std::string& name, const std::string& value);
        virtual         ~field_t();

        std::string     get_name() const;
        bool            has_sub_package_name() const;
        std::string     get_sub_package_name() const;
        std::string     get_field_name() const;

        virtual void    set_value(const std::string& value);
        virtual void    verify_value() const;
        std::string     get_value() const;
        std::string     get_transformed_value() const;

        void            set_filename(const std::string& filename);
        std::string     get_filename() const;

        void            set_line(int line);
        int             get_line() const;

        bool            operator < (const field_t& rhs) const;
        bool            operator == (const field_t& rhs) const;
        field_t&        operator = (const field_t& rhs);

    protected:
        controlled_vars::ptr_auto_init<field_file>  f_field_file;

    private:
        std::string                 f_name;
        std::string                 f_value;
        std::string                 f_filename;
        controlled_vars::zint32_t   f_line;
    };

    typedef std::vector<field_t>        field_list_t;

    class DEBIAN_PACKAGE_EXPORT field_factory_t
    {
    public:
        typedef std::vector<std::string>    name_list_t;

                                    field_factory_t();
        virtual                     ~field_factory_t();

        // each field has a name defined statically like this:
        //static const char *       canonicalized_name();

        virtual char const *        name() const = 0;
        virtual name_list_t         equivalents() const;
        virtual char const *        help() const = 0;
        virtual std::shared_ptr<wpkg_field::field_file::field_t>    create(const field_file& file, const std::string& fullname, const std::string& value) const = 0;

    private:
        // prevent copies
        field_factory_t(const field_factory_t&);
        field_factory_t& operator = (const field_factory_t&);
    };

    class DEBIAN_PACKAGE_EXPORT field_file_state_t
    {
    public:
        virtual ~field_file_state_t();

        virtual bool allow_transformations() const;
        virtual bool accept_sub_packages() const;
    };

    enum write_mode_t
    {
        WRITE_MODE_FIELD_ONLY,
        WRITE_MODE_VARIABLES,
        WRITE_MODE_RAW_FIELDS
    };

    field_file(const std::shared_ptr<field_file_state_t> state);
    std::shared_ptr<field_file_state_t> get_state() const;
    void copy_input(const field_file& source);

    void set_field_variable(const std::string& name, const std::string& value);
    void set_package_name(const std::string& package_name);
    std::string get_package_name() const;
    bool has_sub_packages() const;
    void set_input_file(const memfile::memory_file *input);
    std::string get_filename() const;
    bool read();
    void write(memfile::memory_file& file, write_mode_t write_mode, const list_t& ordered_fields = list_t()) const; //= WRITE_MODE_FIELD_ONLY
    bool eof() const;

    void copy(field_file& destination, const std::string& sub_package, const list_t& excluded) const;
    static std::string output_multiline_field(const std::string& value);

    std::shared_ptr<field_t> create_field(const case_insensitive::case_insensitive_string& name,
                                          const std::string& value,
                                          const std::string& filename = "",
                                          const int line = 0) const;
    std::shared_ptr<field_t> create_variable(const case_insensitive::case_insensitive_string& name,
                                             const std::string& value,
                                             const std::string& filename = "",
                                             const int line = 0) const;

    // basic field handling
    bool                field_is_defined(const std::string& name, bool as_is = false) const;
    int                 number_of_fields() const;
    std::shared_ptr<field_t> get_field_info(const std::string& name) const;
    std::string         get_field(const std::string& name) const;
    const std::string&  get_field_name(int idx) const;
    virtual void        set_field(const std::shared_ptr<field_t> field);
    void                set_field(const std::string& name, const std::string& value);
    void                set_field(const std::string& name, long value);
    bool                delete_field(const std::string& name);
    bool                validate_fields(const std::string& expression);
    void                auto_transform_variables();

    // basic variable handling
    bool                variable_is_defined(const std::string& name) const;
    int                 number_of_variables() const;
    std::shared_ptr<field_t> get_variable_info(const std::string& name) const;
    std::string         get_variable(const std::string& name, const bool substitutions = false) const;
    const std::string&  get_variable_name(int idx) const;
    virtual void        set_variable(const std::shared_ptr<field_t> field);
    void                set_variable(const std::string& name, const std::string& value);
    bool                delete_variable(const std::string& name);
    void                transform_dynamic_variables(const field_t *field, std::string& value) const;

    // specialized field handling
    std::string         get_field_first_line(const std::string& name) const;
    std::string         get_field_long_value(const std::string& name) const;
    list_t              get_field_list(const std::string& name) const;
    bool                get_field_boolean(const std::string& name) const;
    long                get_field_integer(const std::string& name) const;

protected:
    virtual std::shared_ptr<field_t> field_factory(const case_insensitive::case_insensitive_string& name, const std::string& value) const;
    virtual std::shared_ptr<field_t> variable_factory(const case_insensitive::case_insensitive_string& name, const std::string& value) const;
    virtual void verify_file() const;

private:
    // avoid copies
    field_file(const field_file& rhs);
    field_file& operator = (const field_file& rhs);

    friend class field_t;
    virtual std::string replace_variable(const field_t *field, const case_insensitive::case_insensitive_string& name) const;
    static bool validate_standards_version(const std::string& version);
    bool read_field();

    typedef std::map<case_insensitive::case_insensitive_string, std::shared_ptr<field_t>, std::less<case_insensitive::case_insensitive_string> >  field_map_t;
    typedef std::vector<case_insensitive::case_insensitive_string>  field_stack_t;

    field_map_t                         f_fields;
    field_map_t                         f_variables;
    field_map_t                         f_substitutions;

    std::shared_ptr<field_file_state_t> f_state;
    mutable field_stack_t               f_transform_stack;

    // current state while reading an input file
    controlled_vars::ptr_auto_init<const memfile::memory_file>  f_input;
    controlled_vars::zint32_t           f_offset;
    controlled_vars::zint32_t           f_line;
    mutable controlled_vars::zint32_t   f_errcnt;
    std::string                         f_filename;
    std::string                         f_package_name;
    std::string                         f_field_name;
    std::string                         f_field_value;
    controlled_vars::fbool_t            f_is_variable;
    controlled_vars::fbool_t            f_is_reading;
    controlled_vars::fbool_t            f_has_sub_package;
    controlled_vars::fbool_t            f_auto_transform_variables;
};



}       // namespace wpkg_field

#endif
//#ifndef WPKG_FIELD_H
// vim: ts=4 sw=4 et
