#ifndef SQLCOLUMNMATCHER
#define SQLCOLUMNMATCHER
#include "String.h"
#include "SqlWhereClause.h"

namespace metacpp
{
namespace sql
{

typedef struct
{
} null_t;

static const null_t null;

/** Performs conversion of C++ value to SQL literal */
template<typename T, typename = void>
struct ValueEvaluator;

template<typename TObj1, typename TField1, typename TField2,
         typename = typename std::enable_if<std::is_convertible<TField2, TField1>::value>::type>
class SqlColumnAssignment;

template<>
struct ValueEvaluator<String>
{
public:
    String operator()(const String& val) const
    {
        return "\'" + escape(val) + "\'";
    }

private:
    String escape(const String& s) const
    {
        // TODO: escape single quote
        return s;
    }
};

template<>
struct ValueEvaluator<DateTime>
{
public:
    String operator()(const DateTime& val) const
    {
        return "\'" + val.toISOString() + "\'";
    }
};

template<typename T>
struct ValueEvaluator<T, typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
    String operator()(const T& val) const
    {
        return String::fromValue(val);
    }
};

template<typename TField>
class SqlColumnMatcherBase
{
public:
    virtual String expression() const = 0;
};

template<>
class SqlColumnMatcherBase<String>
{
public:
    virtual String expression() const = 0;

    DirectWhereClauseBuilder like(const String& val) const
    {
        return DirectWhereClauseBuilder(this->expression() + " LIKE \'" + val + "\'");
    }
};

/** value expression */
template<typename TField>
class SqlColumnMatcherValue : public SqlColumnMatcherBase<TField>
{
public:
    SqlColumnMatcherValue(const TField& val)
        : m_val(val)
    {
    }

    ~SqlColumnMatcherValue()
    {
    }

    String expression() const override
    {
        ValueEvaluator<TField> eval;
        return eval(m_val);
    }

private:
    TField m_val;
};

/**
    \brief Basic column matcher
*/
template<typename TObj, typename TField>
class SqlColumnMatcherFieldBase : public SqlColumnMatcherBase<TField>
{
public:
    explicit SqlColumnMatcherFieldBase(const MetaField *metaField)
        : m_metaField(metaField)
    {
    }

    const MetaField *metaField() const { return m_metaField; }

    String expression() const override
    {
        return String(TObj::staticMetaObject()->name()) + "." + this->metaField()->name();
    }


private:
    const MetaField *m_metaField;
};

template<typename TField>
class SqlColumnMatcherDirectExpression : public SqlColumnMatcherBase<TField>
{
public:
    explicit SqlColumnMatcherDirectExpression(const String& expr)
        : m_expr(expr)
    {

    }

    ~SqlColumnMatcherDirectExpression()
    {
    }

    String expression() const override
    {
        return m_expr;
    }
private:
    String m_expr;
};

template<typename TObj, typename TField, typename Enable = void>
class SqlColumnPartialMatcher;

template<typename TObj>
class SqlColumnPartialMatcher<TObj, String> :
        public SqlColumnMatcherFieldBase<TObj, String>
{
public:
    explicit SqlColumnPartialMatcher(const MetaField *metaField)
        : SqlColumnMatcherFieldBase<TObj, String>(metaField)
    {
    }
};

template<typename TObj>
class SqlColumnPartialMatcher<TObj, DateTime> :
        public SqlColumnMatcherFieldBase<TObj, DateTime>
{
public:
    explicit SqlColumnPartialMatcher(const MetaField *metaField)
        : SqlColumnMatcherFieldBase<TObj, DateTime>(metaField)
    {
    }
};


template<typename TObj, typename TField>
class SqlColumnPartialMatcher<TObj, TField, typename std::enable_if<std::is_arithmetic<TField>::value>::type> :
        public SqlColumnMatcherFieldBase<TObj, TField>
{
public:
    explicit SqlColumnPartialMatcher(const MetaField *metaField)
        : SqlColumnMatcherFieldBase<TObj, TField>(metaField)
    {
    }
};

template<typename TObj, typename TField>
class SqlColumnFullMatcher
        : public SqlColumnPartialMatcher<TObj, TField>
{
public:
    explicit SqlColumnFullMatcher(const TField TObj::*member)
        : SqlColumnPartialMatcher<TObj, TField>(getMetaField(member))
    {
    }

    // assign other sql subexpression
    template<typename TField2>
    SqlColumnAssignment<TObj, TField, TField2>
    operator=(const SqlColumnMatcherBase<TField2>& rhs)
    {
        return SqlColumnAssignment<TObj, TField, TField2>(*this, rhs);
    }

    // assign a literal
    template<typename TField2>
    SqlColumnAssignment<TObj, TField, TField2>
    operator=(const TField2& rhs)
    {
        return SqlColumnAssignment<TObj, TField, TField2>(*this, SqlColumnMatcherValue<TField2>(rhs));
    }
};


template<typename TObj, typename TField>
class SqlColumnFullMatcher<TObj, Nullable<TField> >
        : public SqlColumnPartialMatcher<TObj, TField>
{
public:
    explicit SqlColumnFullMatcher(const Nullable<TField> TObj::*member)
        : SqlColumnPartialMatcher<TObj, TField>(getMetaField(member))
    {
    }

    DirectWhereClauseBuilder isNull() const
    {
        return DirectWhereClauseBuilder(String(TObj::staticMetaObject()->name()) + "." +
            this->metaField()->name() + " IS NULL");
    }

    DirectWhereClauseBuilder isNotNull() const
    {
        return DirectWhereClauseBuilder(String(TObj::staticMetaObject()->name()) + "." +
            this->metaField()->name() + " IS NOT NULL");
    }

    template<typename TField2>
    typename std::enable_if<std::is_convertible<TField2, TField>::value, SqlColumnAssignment<TObj, TField, TField2>>::type
    operator=(const SqlColumnMatcherBase<TField2>& rhs)
    {
        return SqlColumnAssignment<TObj, TField, TField2>(*this, rhs);
    }

    template<typename TField2>
    typename std::enable_if<std::is_convertible<TField2, TField>::value, SqlColumnAssignment<TObj, TField, TField2>>::type
    operator=(const TField2& rhs)
    {
        return SqlColumnAssignment<TObj, TField, TField2>(*this, SqlColumnMatcherValue<TField2>(rhs));
    }

    SqlColumnAssignment<TObj, TField, TField> operator=(null_t)
    {
        return SqlColumnAssignment<TObj, TField, TField>(*this, SqlColumnMatcherDirectExpression<TField>("NULL"));
    }

    inline DirectWhereClauseBuilder operator==(null_t) const
    {
        return isNull();
    }

    inline DirectWhereClauseBuilder operator!=(null_t) const
    {
        return isNotNull();
    }
};

template<typename TObj, typename TField>
SqlColumnFullMatcher<TObj, TField> GetColumnMatcher(const TField TObj::*member)
{
    return SqlColumnFullMatcher<TObj, TField>(member);
}

#define COL(ColumnSpec) GetColumnMatcher(&ColumnSpec)

template<typename T, typename Enable = void>
struct TypePromotionPriorityHelper;

template<>
struct TypePromotionPriorityHelper<long double>
{
    static constexpr int priority() { return 0; }
};

template<>
struct TypePromotionPriorityHelper<double>
{
    static constexpr int priority() { return 1; }
};

template<>
struct TypePromotionPriorityHelper<float>
{
    static constexpr int priority() { return 2; }
};

template<>
struct TypePromotionPriorityHelper<long long unsigned int>
{
    static constexpr int priority() { return 3; }
};

template<>
struct TypePromotionPriorityHelper<long long int>
{
    static constexpr int priority() { return 4; }
};

template<>
struct TypePromotionPriorityHelper<long unsigned int>
{
    static constexpr int priority() { return 5; }
};

template<>
struct TypePromotionPriorityHelper<long int>
{
    static constexpr int priority() { return 6; }
};

template<>
struct TypePromotionPriorityHelper<unsigned int>
{
    static constexpr int priority() { return 7; }
};

template<>
struct TypePromotionPriorityHelper<int>
{
    static constexpr int priority() { return 8; }
};

// all others are promoted to int
template<typename T>
struct TypePromotionPriorityHelper<T, typename std::enable_if<std::is_arithmetic<T>::value>::type>
{
    static constexpr int priority() { return 1000; }
};

enum _PromotionType
{
    _PromoteToFirst,
    _PromoteToSecond,
    _PromoteToInt
};

template<typename T1, typename T2, _PromotionType>
struct TypePromotionHelper;

template<typename T1, typename T2>
struct TypePromotionHelper<T1, T2, _PromoteToFirst>
{
    typedef T1 PromotionType;
};

template<typename T1, typename T2>
struct TypePromotionHelper<T1, T2, _PromoteToSecond>
{
    typedef T2 PromotionType;
};

template<typename T1, typename T2>
struct TypePromotionHelper<T1, T2, _PromoteToInt>
{
    typedef int PromotionType;
};

template<typename TField1, typename TField2>
struct TypePromotion
{
    typedef typename TypePromotionHelper<TField1, TField2,
    TypePromotionPriorityHelper<TField1>::priority() >= 1000 &&
    TypePromotionPriorityHelper<TField2>::priority() >= 1000 ? _PromoteToInt :
    (TypePromotionPriorityHelper<TField1>::priority() <
     TypePromotionPriorityHelper<TField2>::priority() ?
         _PromoteToFirst : _PromoteToSecond)>::PromotionType Type;
};

//static_assert(std::is_same<typename TypePromotion<int, char>::Type, int>::value, "int with char should promote to int");
//static_assert(std::is_same<typename TypePromotion<short, char>::Type, int>::value, "short with char should promote to int");
//static_assert(std::is_same<typename TypePromotion<short, float>::Type, float>::value, "short with float should promote to float");
//static_assert(std::is_same<typename TypePromotion<float, float>::Type, float>::value, "float with float should promote to float");

#define _INST_REL_OPERATOR(op, sqlop) \
    template<typename TField1, typename TField2> \
    typename std::enable_if<std::is_same<TField1, TField2>::value || \
    (std::is_arithmetic<TField1>::value && \
    std::is_arithmetic<TField2>::value), DirectWhereClauseBuilder>::type \
    operator op(const SqlColumnMatcherBase<TField1>& lhs, \
               const SqlColumnMatcherBase<TField2>& rhs) \
    { \
        return DirectWhereClauseBuilder(lhs.expression() + " " + #sqlop + " " + rhs.expression()); \
    } \
    \
    template<typename TField1, typename TField2> \
    typename std::enable_if<std::is_same<TField1, TField2>::value || \
    (std::is_arithmetic<TField1>::value && \
    std::is_arithmetic<TField2>::value), DirectWhereClauseBuilder>::type \
    operator op(const TField1& lhs, \
               const SqlColumnMatcherBase<TField2>& rhs) \
    { \
        ValueEvaluator<TField1> eval;\
        return DirectWhereClauseBuilder(eval(lhs) + " " #sqlop " " + rhs.expression()); \
    } \
    template<typename TField1, typename TField2> \
    typename std::enable_if<std::is_same<TField1, TField2>::value || \
    (std::is_arithmetic<TField1>::value && \
    std::is_arithmetic<TField2>::value), DirectWhereClauseBuilder>::type \
    operator op(const SqlColumnMatcherBase<TField1>& lhs, \
               const TField2& rhs) \
    { \
        ValueEvaluator<TField2> eval;\
        return DirectWhereClauseBuilder(lhs.expression() + " " + #sqlop + " " + eval(rhs)); \
    } \

_INST_REL_OPERATOR(==, =)
_INST_REL_OPERATOR(!=, !=)
_INST_REL_OPERATOR(>=, >=)
_INST_REL_OPERATOR(<=, <=)
_INST_REL_OPERATOR(>, >)
_INST_REL_OPERATOR(<, <)

#undef _INST_REL_OPERATOR

/** unary - */
template<typename TField>
SqlColumnMatcherDirectExpression<TField>
operator-(const SqlColumnMatcherBase<TField>& inner)
{
    return SqlColumnMatcherDirectExpression<TField>("(-" + inner.expression() + ")");
}

/** unary + */
template<typename TField>
SqlColumnMatcherDirectExpression<TField>
operator+(const SqlColumnMatcherBase<TField>& inner)
{
    return SqlColumnMatcherDirectExpression<TField>("(+" + inner.expression() + ")");
}

#define _INST_BINARY_OPERATOR(op) \
template<typename TField1, typename TField2, typename = typename std::enable_if<std::is_arithmetic<TField1>::value && std::is_arithmetic<TField2>::value>::type> \
SqlColumnMatcherDirectExpression<typename TypePromotion<TField1, TField2>::Type> \
operator op(const SqlColumnMatcherBase<TField1>& lhs, \
          const TField2& rhs) \
{ \
    ValueEvaluator<TField2> eval; \
    return SqlColumnMatcherDirectExpression<typename TypePromotion<TField1, TField2>::Type>( \
        "(" + lhs.expression() + " " #op " " + eval(rhs) + ")"); \
} \
template<typename TField1, typename TField2, typename = typename std::enable_if<std::is_arithmetic<TField1>::value && std::is_arithmetic<TField2>::value>::type> \
SqlColumnMatcherDirectExpression<typename TypePromotion<TField1, TField2>::Type> \
operator op(const TField1& lhs, \
          const SqlColumnMatcherBase<TField2>& rhs) \
{ \
    ValueEvaluator<TField1> eval; \
    return SqlColumnMatcherDirectExpression<typename TypePromotion<TField1, TField2>::Type>("(" + eval(lhs) + " " #op " " + rhs.expression() + ")"); \
} \
template<typename TField1, typename TField2, typename = typename std::enable_if<std::is_arithmetic<TField1>::value && std::is_arithmetic<TField2>::value>::type> \
SqlColumnMatcherDirectExpression<typename TypePromotion<TField1, TField2>::Type> \
operator op(const SqlColumnMatcherBase<TField1>& lhs, \
          const SqlColumnMatcherBase<TField2>& rhs) \
{ \
    return SqlColumnMatcherDirectExpression<typename TypePromotion<TField1, TField2>::Type>( \
                "(" + lhs.expression() + " " #op + " " + rhs.expression() + ")"); \
}

_INST_BINARY_OPERATOR(+)
_INST_BINARY_OPERATOR(-)
_INST_BINARY_OPERATOR(*)
_INST_BINARY_OPERATOR(/)
_INST_BINARY_OPERATOR(%)


#undef _INST_BINARY_OPERATOR

/*** String functions ****/

inline SqlColumnMatcherDirectExpression<String> upper(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<String>("upper(" + column.expression() + ")");
}

inline SqlColumnMatcherDirectExpression<String> lower(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<String>("lower(" + column.expression() + ")");
}

inline SqlColumnMatcherDirectExpression<String> trim(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<String>("trim(" + column.expression() + ")");
}

inline SqlColumnMatcherDirectExpression<String> ltrim(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<String>("ltrim(" + column.expression() + ")");
}

inline SqlColumnMatcherDirectExpression<String> rtrim(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<String>("rtrim(" + column.expression() + ")");
}

inline SqlColumnMatcherDirectExpression<uint64_t> length(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<uint64_t>("length(" + column.expression() + ")");
}

/*** Arihmetic functions  ***/

template<typename TField, typename = typename std::enable_if<std::is_arithmetic<TField>::value>::type >
inline SqlColumnMatcherDirectExpression<TField> abs(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<TField>("abs(" + column.expression() + ")");
}

template<typename TField, typename = typename std::enable_if<std::is_floating_point<TField>::value>::type >
inline SqlColumnMatcherDirectExpression<TField> round(const SqlColumnMatcherBase<String>& column)
{
    return SqlColumnMatcherDirectExpression<TField>("round(" + column.expression() + ")");
}
inline SqlColumnMatcherDirectExpression<int64_t> random()
{
    return SqlColumnMatcherDirectExpression<int64_t>("random()");
}

} // namespace sql
} // namespace metacpp

#endif // SQLCOLUMNMATCHER

