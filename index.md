---
layout: site
name: jbson
tagline: C++ BSON & JSON reader/writer
---
jbson is a library for building & iterating BSON data, and JSON documents in C++14.

## Features
* UTF-8 support only (currently).
* Header only.
* Depends only on Boost headers & C++14 standard library. [note](#notes)
* Fully compatible with documented BSON specs.
* A strict, iterator-based JSON parser. Mostly compliant with JSON standard. [note](#notes)
* Parses [MongoDB extended JSON](http://docs.mongodb.org/manual/reference/mongodb-extended-json/).
* User-defined JSON literals `""_json_set`, `""_json_doc`, `""_json_arr`.
* An implementation of [JSONPath](http://goessner.net/articles/JsonPath/), with some support for filtering with boolean expressions.

## Usage

All types are under the `jbson` namespace.

### Elements

`basic_element` is the class through which elements' names & values are accessed. The names are *always* copied (may change in the future). Value data may or may not be, depending on the container template parameter.  
The type of an element can be determined through comparison with the `element_type` enum class values.  
Values can be accessed through the `value()` member function or the `get()` free function.

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

Elements can be modified using the `value()` member function.

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

Elements can also be accessed via the visitor pattern.

    using namespace jbson;
    element elem;
    //...
    struct Visitor {
        template <typename T>
        void operator()(boost::string_ref name, T&& value, element_type etype) const {
            // not sure why the parameters are in this order
        }

        void operator()(boost::string_ref name, element_type etype) const {
            // for elements whose type cannot have a value
            // e.g. null_element, undefined_element
        }
    };
    elem.visit(Visitor{});

Visitors can also return values. It is not necessary to typedef a `return_type`.

    using namespace jbson;
    element elem;
    //...
    struct IsNull {
        template <typename T>
        bool operator()(boost::string_ref, T&&, element_type) const {
            return false;
        }
        void operator()(boost::string_ref, element_type) const {
            return true;
        }
    };
    bool null = elem.visit(IsNull{});

### Building Documents & Arrays

`builder` & `array_builder` are simple classes used for building documents & arrays, respectively.
These allow for documents & arrays to be built from variables, rather than just literals.

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

While the intent of this is clear, it's a little verbose due to the explicit type.
This can be omitted when the type is compatible with a JSON type, excluding documents and arrays.
The above example becomes:

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
        ("some obj", element_type::document_element, builder
            ("child bool", false)
        );

That's about as good as it's going to get. `array_builder` is used the same way, apart from the name parameter is an integer and optional.

    using namespace jbson;
    auto str = "str"s;
    int num = 123;
    // ["str", 123, { "child bool": false }]
    array arr = array_builder
        (str)
        (num)
        (element_type::document_element, builder
            ("child bool", false)
        );

### Documents

The main classes of jbson are the templates `basic_document`, `basic_array` & `basic_element`. These types are parameterised with their underlying container or range. It's generally recommended to use a random-access range. Their default aliases i.e. `document` & `element` are parameterised with `std::vector<char>`. The second template parameter of `basic_document` defines the parameter of child `basic_element`s. By default this is defined as a `boost::iterator_range` to avoid copying data, though care should be taken that this element does **not** outlive the container/document which owns the data, else it can be copied to its own element.

    jbson::document doc;
    //...
    for(auto&& element: doc) {
        //...
    }
    std::for_each(doc.begin(), doc.end(), [](auto&& v) { something(v); });

`basic_array` is implemented using `basic_document` and the classes can usually be interchanged freely. Of course, `basic_document` is for documents and `basic_array` is for arrays and should be used as such.

`basic_document` is simply a constant wrapper around its BSON data container, allowing iteration of elements in the form of a `basic_element`. It can be used with C++11's range-based for loop, and non-modifying standard algorithms.  
Iterators retain a copy of elements. This does not mean that an iterator contains a copy of an element's data, however.

### Document Set

`basic_document_set` is an alias template to `std::multi_set<basic_element,...>` which `basic_document` and `basic_array` is explicitly convertible to. This can be used to modify an existing document, at the cost of copying/converting the entire document into its constituent elements. The type parameter of `basic_document_set` should be a *container* not a *range*, i.e. it should own the data. The default alias `document_set` is `basic_document_set<std::vector<char>>` making it a set of `basic_element<std::vector<char>>`.

    jbson::document doc;
    //...
    jbson::document_set set(doc);
    //... modify set
    doc = set; // convert back

### JSON Parsing

JSON documents can be parse with the `json_reader` class, which parses directly to BSON data and can be implicitly converted to any valid `basic_document`, `basic_array` or `basic_document_set`. It can also be move-converted to a `document` or `array` for efficient conversion.

    jbson::json_reader reader{};
    reader.parse("{\"some json\": 123 }");
    reader.parse(R"({"some json": 123 })"); // C++11 raw string
    auto doc = jbson::document(std::move(reader));

`json_reader` can parse string literals, or any range of UTF-8 codepoints e.g. `std::string`, `boost::string_ref`, `boost::iterator_range<...>`, `QByteArray`, etc.  
For convenience, user-defined literals have been implemented for JSON documents.

    using jbson::literal;
    auto doc = R"({
        "some json": 123
    })"_json_doc;

This is equivalent to the above example.

## Performance
Performance of the JSON parser is decent, as measured by [json_benchmark](https://github.com/mloskot/json_benchmark), it's 2nd only to rapidjson:

> QJsonDocument.small: 1000 iterations of 500 parsings in 8.36422 to 8.36658 sec based on 2 benchmarks  
> QJsonDocument.large: 1000 iterations of 1 parsings in 311.287 to 311.843 sec based on 2 benchmarks

> jsoncpp.small: 1000 iterations of 500 parsings in 13.7716 to 13.782 sec based on 2 benchmarks  
> jsoncpp.large: 1000 iterations of 1 parsings in 283.477 to 284.996 sec based on 2 benchmarks

> rapidjson.small: 1000 iterations of 500 parsings in 0.99631 to 1.00371 sec based on 2 benchmarks  
> rapidjson.large: 1000 iterations of 1 parsings in 13.4129 to 13.4314 sec based on 2 benchmarks

> jbson.small: 1000 iterations of 500 parsings in 4.88315 to 4.89001 sec based on 2 benchmarks  
> jbson.large: 1000 iterations of 1 parsings in 131.038 to 131.099 sec based on 2 benchmarks

> jsoncons.small: 1000 iterations of 500 parsings in 10.8905 to 10.9053 sec based on 2 benchmarks  
> jsoncons.large: 1000 iterations of 1 parsings in 551.351 to 552.098 sec based on 2 benchmarks

## Notes

jbson uses BSON-like terminology. Eg. a document is not only the root object, but also all embedded objects.

BSON is incompatible with JSON in that element names cannot contain `'\0'`, making names not fully UTF-8 compliant, which JSON requires.

Requires `<codecvt>` header (C++11, missing in gcc-4.8's stdlib), various template aliases (C++14), probably more where gcc's stdlib is behind on the standard.  
Known only to work with an up-to-date libc++ currently.
