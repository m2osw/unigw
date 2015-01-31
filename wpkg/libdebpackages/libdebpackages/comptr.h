// Copyright (c) 2012-2015  Made to Order Software Corporation
// This code is heavily based on code by Si Brown found here:
// http://www.flipcode.com/archives/COM_Smart_Pointer.shtml
#ifndef COMPTR_H
#define COMPTR_H

/** \file
 * \brief A simple COM pointer.
 *
 * This class is an RAII COM pointer which automatically does the
 * necessary AddRef() and Release() calls.
 */

/** \brief A template COM smart pointer.
 *
 * This class is not thread safe.
 */
template<typename I>
class ComPtr
{
public:
    /** \brief Constructs an empty smart pointer.
     *
     * The default constructor creates a NULL smart pointer.
     */
    ComPtr()
        : m_p(0)
    {
    }

    /** \brief Assumes ownership of the given instance, if non-null.
     *
     * The smart pointer will assume ownership of the given instance.
     * It will \b not AddRef the contents, but it will Release the object
     * as it goes out of scope.
     *
     * \param[in]  p    A pointer to an existing COM instance, or 0 to create
     *                  an empty COM smart pointer.
     */
    explicit ComPtr(I* p)
        : m_p(p)
    {
    }

    /** \brief Releases the contained instance.
     *
     * The destructor ensures that the interface gets released safely.
     *
     * \sa SafeRelease()
     */
    ~ComPtr()
    {
        SafeRelease();
    }

    /** \brief Copy construction.
     *
     * Copy a a COM pointer from a smart pointer to another.
     * In this case the interface AddRef() gets called.
     */
    ComPtr(ComPtr<I> const& ptr)
        : m_p(ptr.m_p)
    {
        if(m_p)
        {
            m_p->AddRef();
        }
    }

    /** \brief Assignment operator.
     *
     * This function safely copies a smart pointer in this
     * pointer.
     */
    ComPtr<I>& operator = (ComPtr<I> const& ptr)
    {
        ComPtr<I> copy(ptr);
        Swap(copy);
        return *this;
    }

    /** \brief Releases a contained instance, if present.
     *
     * You should never need to call this function unless you wish to
     * Release an instance before the smart pointer goes out of scope.
     */
    void SafeRelease()
    {
        if(m_p != NULL)
        {
            m_p->Release();
            m_p = NULL;
        }
    }

    /** \brief Explicitly gets the address of the pointer.
     *
     * This function is used to get the pointer of the pointer which is used
     * to retrieve the interface (i.e. a QueryInterface() call). This is why
     * the function returns a void ** pointer.
     *
     * \code
     * ComPtr<IPersistFile> persist_file;
     * HRESULT hr = shell_link->QueryInterface(IID_IPersistFile, persist_file.AdressOf());
     * \endcode
     *
     * This function releases the current pointer if there is one.
     *
     * \return A pointer to the interface pointer held by this class.
     *
     * \sa SafeRelease()
     */
    void **AddressOf()
    {
        SafeRelease();
        return reinterpret_cast<void **>(&m_p);
    }

    /** \brief Gets the encapsulated pointer.
     *
     * Explicitly get the interface pointer.
     *
     * \return The interface pointer.
     */
    I* Get() const
    {
        return m_p;
    }

    /** \brief Gets the encapsulated pointer.
     *
     * Get the interface pointer using the pointer operator.
     *
     * \return The interface pointer.
     */
    I* operator -> () const
    {
        return m_p;
    }

    /** \brief Swaps the encapsulated pointer with that of the argument.
     *
     * This function swaps the pointers of the this smart pointer and
     * the specified ptr smart pointer.
     *
     * \param[in,out] ptr  The pointer to swap with this pointer.
     */
    void Swap(ComPtr<I>& ptr)
    {
        I* p = m_p;
        m_p = ptr.m_p;
        ptr.m_p = p;
    }

    /** \brief Returns true if non-empty.
     *
     * \return Returns true if the interface pointer is not NULL.
     */
    operator bool() const
    {
        return m_p != NULL;
    }

private:
    /** \brief The encapsulated instance.
     *
     * This is the instance pointer.
     */
    I* m_p;
};

#endif
// #ifndef COMPTR_H
// vim: ts=4 sw=4 et
