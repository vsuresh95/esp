#ifndef __AVU_MON_INFO_HPP__
#define __AVU_MON_INFO_HPP__

#define MODE_BITS 2

class avu_mon_info_t
{
    public:
        // Monitor data
        uint32_t data;

        // Monitor mode
        sc_dt::sc_bv<MODE_BITS> mode;

        // Constructors
        avu_mon_info_t()
            : data(0), mode(0) { }

        avu_mon_info_t(uint32_t d, sc_dt::sc_bv<MODE_BITS> m)
            : data(d), mode(m) { }

        avu_mon_info_t(const avu_mon_info_t &other)
            : data(other.data), mode(other.mode) { }

        // Assign operator
        inline avu_mon_info_t& operator=(const avu_mon_info_t &other)
        {
            data = other.data;
            mode = other.mode;
            return *this;
        }

        // Equals operator
        inline bool operator==(const avu_mon_info_t &rhs) const
        {
            return ((rhs.data == data) && (rhs.mode == mode));
        }

        // Dump operator
        friend ostream& operator<<(ostream& os, avu_mon_info_t const &avu_mon_info)
        {
            os << "{" << avu_mon_info.data  << ","
                    << avu_mon_info.mode << "}";
            return os;
        }

        // Makes this type traceable by SystemC
        friend void sc_trace(sc_trace_file *tf, const avu_mon_info_t &v, const std::string &name) { }
};

#endif // __AVU_MON_INFO_HPP__
