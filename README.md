jbson is a library for building & iterating BSON data, and JSON documents in C++14.

\tableofcontents

# Features # {#features}

* Header only. Boost licensed.
* Depends only on Boost headers & C++14 standard library. [note 3](#notes)
* Fully compatible with documented BSON specs.
* A strict, iterator-based JSON parser. Mostly compliant with JSON standard. [note 2](#notes)
* Parses [MongoDB extended JSON](http://docs.mongodb.org/manual/reference/mongodb-extended-json/) from UTF-8,16,32 strings.
* User-defined JSON literals `""_json_set`, `""_json_doc`, `""_json_arr`.
* An implementation of [JSONPath](http://goessner.net/articles/JsonPath/), with some support for filtering with boolean expressions.

# Usage # {#usage}

All types and functions are under the [jbson](@ref jbson) namespace.

## Elements {#elements}

`jbson::basic_element` is the class through which elements' names & values are accessed and has a template parameter for its data container. Generally, this will be a non-owning range (e.g. `boost::iterator_range`), but there is an `jbson::element` alias to `jbson::basic_element<std::vector<char>>` to enforce data ownership. The names are *always* copied (may change in the future). Value data may or may not be, depending on the container template parameter.  
The type of an element can be determined through comparison with the `jbson::element_type` enum class values.  
Values can be accessed through the `jbson::basic_element::value()` member function or the `jbson::get()` free function.

~~~cpp
    using jbson::element_type;
    jbson::element elem;
    //...
    if(elem.type() == element_type::string_element) {
        auto str = elem.value<std::string>();
        //OR
        auto str = jbson::get<std::string>(elem);
        //OR if container is contiguous
        auto str = elem.value<boost::string_ref>();
        //OR
        auto str = jbson::get<boost::string_ref>(elem);
        //OR returns either std::string or boost::string_ref, depending on container
        auto str = jbson::get<element_type::string_element>(elem);
    }
~~~

Elements can be modified using the `value()` member function.

~~~cpp
    using jbson::element_type;
    jbson::element elem;
    elem.value("some string");
    assert(elem.type() == element_type::string_element);
    elem.value(123);
    assert(elem.type() == element_type::int32_element);
    elem.value<bool>(123);
    assert(elem.type() == element_type::boolean_element);
    elem.value<element_type::boolean_element>(123);
    assert(elem.type() == element_type::boolean_element);
    elem.value(element_type::boolean_element, 123);
    assert(elem.type() == element_type::boolean_element);
~~~

Elements can also be accessed via the visitor pattern.

~~~cpp
    using namespace jbson;
    element elem;
    //...
    struct Visitor {
        template <typename T>
        void operator()(boost::string_ref name, element_type etype, T&& value) const {
            // do something
        }

        void operator()(boost::string_ref name, element_type etype) const {
            // for elements whose type cannot have a value
            // e.g. null_element, undefined_element
        }
    };
    elem.visit(Visitor{});
~~~

Visitors can also return values. It is not necessary to typedef a `return_type`.

~~~cpp
    using namespace jbson;
    element elem;
    //...
    struct IsNull {
        template <typename T>
        bool operator()(boost::string_ref, element_type, T&&) const {
            return false;
        }
        bool operator()(boost::string_ref, element_type) const {
            return true;
        }
    };
    bool null = elem.visit(IsNull{});
~~~

## Building Documents & Arrays {#building}

`builder` & `array_builder` are simple classes used for building documents & arrays, respectively.
These allow for documents & arrays to be built from variables, rather than just literals.

~~~cpp
    using namespace jbson;
    auto str = "str"s;
    int num = 123;
    // {
    //     "some string": "str",
    //     "some int": 123,
    //     "some obj": {
    //         "child bool": false
    //     }
    // }
    document doc = builder
        ("some string", element_type::string_element, str)
        ("some int", element_type::int32_element, num)
        ("some obj", element_type::document_element, builder
            ("child bool", element_type::boolean_element, false)
        );
~~~

While the intent of this is clear, it's a little verbose due to the explicit type.
This can be omitted when the type is compatible with a JSON type.
The above example becomes:

~~~cpp
    using namespace jbson;
    auto str = "str"s;
    int num = 123;
    // {
    //     "some string": "str",
    //     "some int": 123,
    //     "some obj": {
    //         "child bool": false
    //     }
    // }
    document doc = builder
        ("some string", str)
        ("some int", num)
        ("some obj", builder
            ("child bool", false)
        );
~~~

That's about as good as it's going to get. `array_builder` is used the same way, apart from the lack of a name parameter, which is an implicitly set integer.

~~~cpp
    using namespace jbson;
    auto str = "str"s;
    int num = 123;
    // ["str", 123, { "child bool": false }]
    array arr = array_builder
        (str)
        (num)
        (builder
            ("child bool", false)
        );
~~~

## Documents {#docs}

The main classes of jbson are the templates `jbson::basic_document`, `jbson::basic_array` & `jbson::basic_element`. These types are parameterised with their underlying container or range. It's generally recommended to use a random-access range. Their default aliases i.e. `document` & `element` are parameterised with `std::vector<char>`. The second template parameter of `basic_document` defines the parameter of child `basic_element`s. By default this is defined as a `boost::iterator_range` to avoid copying data, though care should be taken that this element does **not** outlive the container/document which owns the data, else it can be copied to its own `jbson::element`.

~~~cpp
    jbson::document doc;
    //...
    for(auto&& element: doc) {
        //...
    }
    std::for_each(doc.begin(), doc.end(), [](auto&& v) { something(v); });
~~~

`basic_array` is implemented using `basic_document` and the classes can usually be interchanged freely. Of course, `basic_document` is for documents and `basic_array` is for arrays and should be used as such.

`basic_document` is simply a constant wrapper around its BSON data container, allowing iteration of elements in the form of a `basic_element`. It can be used with C++11's range-based for loop, and non-modifying standard algorithms.  
Iterators retain a copy of elements. This does not mean that an iterator contains a copy of an element's data, however.

## Document Set {#doc_set}

`basic_document_set` is an alias template to `std::set<basic_element,...>` which `basic_document` and `basic_array` are explicitly convertible to. This can be used to easily modify an existing document, at the cost of copying/converting the entire document into its constituent elements. The type parameter of `basic_document_set` should be a *container* not a *range*, i.e. it should own the data. The default alias `document_set` is `basic_document_set<std::vector<char>>` making it a set of `basic_element<std::vector<char>>`.

~~~cpp
    jbson::document doc;
    //...
    jbson::document_set set(doc);
    //... modify set
    doc = set; // convert back
~~~

## JSON Parsing {#json}

JSON documents can be parse with the `json_reader` class, which parses directly to BSON data and can be implicitly converted to any valid `basic_document`, `basic_array` or `basic_document_set`. It can also be move-converted to a `document` or `array` for efficient conversion.

~~~cpp
    jbson::json_reader reader{};
    reader.parse("{\"some json\": 123 }");
    reader.parse(R"({"some json": 123 })"); // C++11 raw string
    auto doc = jbson::document(std::move(reader));
~~~

`json_reader` can parse string literals, or any range of UTF-(8, 16 or 32) characters e.g. `std::string`, `boost::string_ref`, `boost::iterator_range<...>`, `QByteArray`, etc. To parse UTF-16 or UTF-32 documents, `json_reader` must be passed a range of (or iterators to) `char16_t` or `char32_t`, respectively. It is assumed the input code units use the system endian, and a BOM (byte-order mark) at the beginning of input is an error. `json_reader` may also work with `wchar_t`, though this usage is non-standard, non-portable and unsupported.  
For convenience, user-defined literals have been implemented for JSON documents.

~~~cpp
    using jbson::literal;
    auto doc = R"({
        "some json": 123
    })"_json_doc;
~~~

This is equivalent to the above example.

# Performance {#performance}

Performance of the JSON parser is decent, as measured by [json_benchmark](https://github.com/mloskot/json_benchmark), it's 2nd only to rapidjson (or 3rd to QJsonDocument with small input):

> QJsonDocument.small: 1000 iterations of 500 parsings in 1.04621 to 1.05643 sec based on 2 benchmarks  
> QJsonDocument.large: 1000 iterations of 1 parsings in 58.1027 to 58.1235 sec based on 2 benchmarks

> jsoncpp.small: 1000 iterations of 500 parsings in 2.4124 to 2.45696 sec based on 2 benchmarks    
> jsoncpp.large: 1000 iterations of 1 parsings in 62.7781 to 62.7823 sec based on 2 benchmarks

> rapidjson.small: 1000 iterations of 500 parsings in 0.454047 to 0.48559 sec based on 2 benchmarks  
> rapidjson.large: 1000 iterations of 1 parsings in 4.8566 to 4.86322 sec based on 2 benchmarks

> jbson.small: 1000 iterations of 500 parsings in 1.24328 to 1.27775 sec based on 2 benchmarks  
> jbson.large: 1000 iterations of 1 parsings in 43.0072 to 43.0377 sec based on 2 benchmarks

> jsoncons.small: 1000 iterations of 500 parsings in 10.2386 to 10.2747 sec based on 2 benchmarks  
> jsoncons.large: 1000 iterations of 1 parsings in 243.634 to 243.796 sec based on 2 benchmarks

Some more measurements using <https://github.com/sanSS/json-bechmarks>.

## Small Objects

Parser        | Average Time (us) | Memory Usage
------------- | ----------------- | ------------
jsoncpp       | 9411              | 4,084 K
QJsonDocument | 7009              | 3,684 K
jbson         | 5981              | 1,132 K

## Medium Objects

Parser        | Average Time (us) | Memory Usage
------------- | ----------------- | ------------
jsoncpp       | 121035            | 44,224 K
QJsonDocument | 82839             | 37,996 K
jbson         | 64800             | 24,708 K

## Large Objects

Parser        | Average Time (us) | Memory Usage
------------- | ----------------- | ------------
jsoncpp       | 438745            | 131,204 K
QJsonDocument | DNF               | N/A
jbson         | 190883            | 70,7748 K

# Notes {#notes}

1. jbson uses BSON-like terminology. Eg. a document is not only the root object, but also all embedded objects.
2. BSON is incompatible with JSON in that element names cannot contain '\0', making names not fully UTF-8 compliant, which JSON requires.
3. Requires codecvt header (C++11, missing in gcc-4.8 & gcc-4.9 stdlib), various template aliases (C++14), probably more where gcc's stdlib is behind on the standard.  
Known only to work with an up-to-date libc++ currently.
