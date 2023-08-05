
#include "units.hpp"

units time_units_us = 
{
    1000,
    "us",
    { "ms", "s", "" }
};

units time_units_s = 
{
    60,
    "s",
    { "m", "h", "" }
};

units binary_units = 
{
    1024,
    "",
    { "K", "M", "G", "T", "P", "" }
};

units metric_units = 
{
    1000,
    "",
    { "k", "M", "G", "T", "P", "" }
};

std::string formatUnits(long double n, units& m, int p) 
{
    long double amt = n, scale;
    std::string unit = m.base;
    
    scale = m.scale * 0.85;

    for (int i = 0; i < m.values.size() - 1 && amt >= scale; i++) 
    {
        amt /= m.scale;
        unit = m.values[i];
    }

    std::stringstream formatter;
    formatter.precision(p);
    formatter << std::fixed << amt << unit;
    
    return formatter.str();
}

int scanUnits(std::string s, uint64_t& n, units& m)
{
    uint64_t base, scale = 1;
    std::string unit;
    int i, c;

    size_t base_end = s.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if (base_end == std::string::npos)
    {
        n = std::stoull(s);
        return 0;
    }

    base = std::stoull(s.substr(0, base_end));
    unit = s.substr(base_end);

    if (!s.empty() && std::find(m.values.begin(), m.values.end(), unit) != m.values.end())
    {
        for (i = 0; i < m.values.size(); i++) 
        {
            scale *= m.scale;
            if (unit == m.values[i])
                break;
        }

        if (i == m.values.size())
            return 2;
    }

    n = base * scale;
    return 0;
}

std::string formatBinary(long double n) 
{
    return formatUnits(n, binary_units, 2);
}

std::string formatMetric(long double n, int p) 
{
    return formatUnits(n, metric_units, p);
}

std::string formatTime_us(long double n, int p)
{
    units units = time_units_us;
    if (n >= 1000000.0) 
    {
        n /= 1000000.0;
        units = time_units_s;
    }

    return formatUnits(n, units, p);
}

std::string formatTime_s(long double n) 
{
    return formatUnits(n, time_units_s, 0);
}

int scanMetric(std::string s, uint64_t& n) 
{
    return scanUnits(s, n, metric_units);
}

int scanTime(std::string s, uint64_t& n) 
{
    return scanUnits(s, n, time_units_s);
}
