#!/usr/bin/env ruby
# Class for packed binary data, with defined bitfields and accessors for them.
# See {intro.txt}[link:../doc/files/intro_txt.html] for an overview.
#
# Data after the end of the defined fields is accessible using the +rest+
# declaration. See examples/ip.rb. Nested fields can be declared using +nest+.
# See examples/nest.rb.
#
# Note that all string methods are still available: length, grep, etc.
# The String#replace method is useful.
#
class BitStruct < String

  class Field
    # Offset of field in bits.
    attr_reader :offset
    
    # Length of field in bits.
    attr_reader :length
    alias size length
    
    # Name of field (used for its accessors).
    attr_reader :name
    
    # Options, such as :default (varies for each field subclass).
    # In general, options can be provided as strings or as symbols.
    attr_reader :options
    
    # Display name of field (used for printing).
    attr_reader :display_name
    
    # Default value.
    attr_reader :default
    
    # Format for printed value of field.
    attr_reader :format
    
    # Subclasses can override this to define a default for all fields of this
    # class, not just the one currently being added to a BitStruct class, a
    # "default default" if you will. The global default, if #default returns
    # nil, is to fill the field with zero. Most field classes just let this
    # default stand. The default can be overridden per-field when a BitStruct
    # class is defined.
    def self.default; nil; end

    # Used in describe.
    def self.class_name
      @class_name ||= name[/\w+$/]
    end
    
    # Used in describe. Can be overridden per-subclass, as in NestedField.
    def class_name
      self.class.class_name
    end

    # Yield the description of this field, as an array of 5 strings: byte
    # offset, type, name, size, and description. The opts hash may have:
    #
    # :expand ::  if the value is true, expand complex fields
    #
    # (Subclass implementations may yield more than once for complex fields.)
    #
    def describe opts
      bits = size
      if bits > 32 and bits % 8 == 0
        len_str = "%dB" % (bits/8)
      else
        len_str = "%db" % bits
      end
      
      byte_offset = offset / 8 + (opts[:byte_offset] || 0)

      yield ["@%d" % byte_offset, class_name, name, len_str, display_name]
    end

    # Options are _display_name_, _default_, and _format_ (subclasses of Field
    # may add other options).
    def initialize(offset, length, name, opts = {})
      @offset, @length, @name, @options =
        offset, length, name, opts
      
      @display_name = opts[:display_name] || opts["display_name"]
      @default      = opts[:default] || opts["default"] || self.class.default
      @format       = opts[:format] || opts["format"]
    end
    
    # Inspect the value of this field in the specified _obj_.
    def inspect_in_object(obj, opts)
      val = obj.send(name)
      str =
        begin
          val.inspect(opts)
        rescue ArgumentError # assume: "wrong number of arguments (1 for 0)"
          val.inspect
        end
      (f=@format) ? (f % str) : str
    end
    
    # Normally, all fields show up in inspect, but some, such as padding,
    # should not.
    def inspectable?; true; end
  end
  
  NULL_FIELD = Field.new(0, 0, :null, :display_name => "null field")
  
  # Raised when a field is added after an instance has been created. Fields
  # cannot be added after this point.
  class ClosedClassError < StandardError; end

  # Raised if the chosen field name is not allowed, either because another
  # field by that name exists, or because a method by that name exists.
  class FieldNameError < StandardError; end
  
  @default_options = {}
    
  class << self
    # ------------------------
    # :section: field access methods
    #
    # For introspection and metaprogramming.
    #
    # ------------------------

    # Return the list of fields for this class.
    def fields
      @fields ||= self == BitStruct ? [] : superclass.fields.dup
    end
    
    # Return the list of fields defined by this class, not inherited
    # from the superclass.
    def own_fields
      @own_fields ||= []
    end

    # Add a field to the BitStruct (usually, this is only used internally).
    def add_field(name, length, opts = {})
      round_byte_length ## just to make sure this has been calculated
      ## before adding anything
      
      name = name.to_sym
      
      if @closed
        raise ClosedClassError, "Cannot add field #{name}: " +
          "The definition of the #{self.inspect} BitStruct class is closed."
      end

      if fields.find {|f|f.name == name}
        raise FieldNameError, "Field #{name} is already defined as a field."
      end

      if instance_methods(true).find {|m| m == name}
        if opts[:allow_method_conflict] || opts["allow_method_conflict"]
          warn "Field #{name} is already defined as a method."
        else
          raise FieldNameError,"Field #{name} is already defined as a method."
        end
      end
      
      field_class = opts[:field_class]
      
      prev = fields[-1] || NULL_FIELD
      offset = prev.offset + prev.length
      field = field_class.new(offset, length, name, opts)
      field.add_accessors_to(self)
      fields << field
      own_fields << field
      @bit_length += field.length
      @round_byte_length = (bit_length/8.0).ceil

      if @initial_value
        diff = @round_byte_length - @initial_value.length
        if diff > 0
          @initial_value << "\0" * diff
        end
      end

      field
    end

    def parse_options(ary, default_name, default_field_class) # :nodoc:
      opts = ary.grep(Hash).first || {}
      opts = default_options.merge(opts)
      
      opts[:display_name]  = ary.grep(String).first || default_name
      opts[:field_class]   = ary.grep(Class).first || default_field_class
      
      opts
    end
    
    # Get or set the hash of default options for the class, which apply to all
    # fields. Changes take effect immediately, so can be used alternatingly with
    # blocks of field declarations. If +h+ is provided, update the default
    # options with that hash. Default options are inherited.
    #
    # This is especially useful with the <tt>:endian => val</tt> option.
    def default_options h = nil
      @default_options ||= superclass.default_options.dup
      if h
        @default_options.merge! h
      end
      @default_options
    end
    
    # Length, in bits, of this object.
    def bit_length
      @bit_length ||= fields.inject(0) {|a, f| a + f.length}
    end
    
    # Length, in bytes (rounded up), of this object.
    def round_byte_length
      @round_byte_length ||= (bit_length/8.0).ceil
    end
    
    def closed! # :nodoc:
      @closed = true
    end

    def field_by_name name
      @field_by_name ||= {}
      field = @field_by_name[name]
      unless field
        field = fields.find {|f| f.name == name}
        @field_by_name[name] = field if field
      end
      field
    end
  end
  
  # Return the list of fields for this class.
  def fields
    self.class.fields
  end
  
  # Return the field with the given name.
  def field_by_name name
    self.class.field_by_name name
  end

  # ------------------------
  # :section: metadata inspection methods
  #
  # Methods to textually describe the format of a BitStruct subclass.
  #
  # ------------------------

  class << self
    # Default format for describe. Fields are byte, type, name, size,
    # and description.
    DESCRIBE_FORMAT = "%8s: %-12s %-14s[%4s] %s"
    
    # Can be overridden to use a different format.
    def describe_format
      DESCRIBE_FORMAT
    end

    # Textually describe the fields of this class of BitStructs.
    # Returns a printable table (array of line strings), based on +fmt+,
    # which defaults to #describe_format, which defaults to +DESCRIBE_FORMAT+.
    def describe(fmt = nil, opts = {})
      if block_given?
        fields.each do |field|
          field.describe(opts) do |desc|
            yield desc
          end
        end
        nil
        
      else
        fmt ||= describe_format

        result = []

        unless opts[:omit_header]
          result << fmt % ["byte", "type", "name", "size", "description"]
          result << "-"*70
        end

        fields.each do |field|
          field.describe(opts) do |desc|
            result << fmt % desc
          end
        end

        unless opts[:omit_footer]
          result << @note if @note
        end

        result
      end
    end
    
    # Subclasses can use this to append a string (or several) to the #describe
    # output. Notes are not cumulative with inheritance. When used with no
    # arguments simply returns the note string
    def note(*str)
      @note = str unless str.empty?
      @note
    end
  end
  
  # ------------------------
  # :section: initialization and conversion methods
  #
  # ------------------------

  # Initialize the string with the given string or bitstruct, or with a hash of
  # field=>value pairs, or with the defaults for the BitStruct subclass. Fields
  # can be strings or symbols. Finally, if a block is given, yield the instance
  # for modification using accessors.
  def initialize(value = nil)   # :yields: instance
    self << self.class.initial_value

    case value
    when Hash
      value.each do |k, v|
        send "#{k}=", v
      end
    
    when nil
      
    else
      self[0, value.length] = value
    end
    
    self.class.closed!
    yield self if block_given?
  end
  
  DEFAULT_TO_H_OPTS = {
    :convert_keys   => :to_sym,
    :include_rest   => true
  }
  
  # Returns a hash of {name=>value,...} for each field. By default, include
  # the rest field.
  # Keys are symbols derived from field names using +to_sym+, unless
  # <tt>opts[:convert_keys]<\tt> is set to some other method name.
  def to_h(opts = DEFAULT_TO_H_OPTS)
    converter = opts[:convert_keys] || :to_sym

    fields_for_to_h = fields
    if opts[:include_rest] and (rest_field = self.class.rest_field)
      fields_for_to_h += [rest_field]
    end
    
    fields_for_to_h.inject({}) do |h,f|
      h[f.name.send(converter)] = send(f.name)
      h
    end
  end
  
  # Returns an array of values of the fields of the BitStruct. By default,
  # include the rest field.
  def to_a(include_rest = true)
    ary =
      fields.map do |f|
        send(f.name)
      end
    
    if include_rest and (rest_field = self.class.rest_field)
      ary << send(rest_field.name)
    end
  end
  
  class << self
    # The unique "prototype" object from which new instances are copied.
    # The fields of this instance can be modified in the class definition
    # to set default values for the fields in that class. (Otherwise, defaults
    # defined by the fields themselves are used.) A copy of this object is
    # inherited in subclasses, which they may override using defaults and
    # by writing to the initial_value object itself.
    #
    # If called with a block, yield the initial value object before returning
    # it. Useful for customization within a class definition.
    #
    def initial_value   # :yields: the initial value
      unless @initial_value
        iv = defined?(superclass.initial_value) ? 
          superclass.initial_value.dup : ""
        if iv.length < round_byte_length
          iv << "\0" * (round_byte_length - iv.length)
        end

        @initial_value = "" # Serves as initval while the real initval is inited
        @initial_value = new(iv)
        @closed = false # only creating the first _real_ instance closes.
        
        fields.each do |field|
          @initial_value.send("#{field.name}=", field.default) if field.default
        end
      end
      yield @initial_value if block_given?
      @initial_value
    end
    
    # Take +data+ (a string or BitStruct) and parse it into instances of
    # the +classes+, returning them in an array. The classes can be given
    # as an array or a separate arguments. (For parsing a string into a _single_
    # BitStruct instance, just use the #new method with the string as an arg.)
    def parse(data, *classes)
      classes.flatten.map do |c|
        c.new(data.slice!(0...c.round_byte_length))
      end
    end
    
    # Join the given structs (array or multiple args) as a string.
    # Actually, the inherited String#+ instance method is the same, as is using
    # Array#join.
    def join(*structs)
      structs.flatten.map {|struct| struct.to_s}.join("")
    end
  end

  # ------------------------
  # :section: inspection methods
  #
  # ------------------------

  DEFAULT_INSPECT_OPTS = {
    :format           => "#<%s %s>",
    :field_format     => "%s=%s",
    :separator        => ", ",
    :field_name_meth  => :name,
    :include_rest     => true
  }
  
  DETAILED_INSPECT_OPTS = {
    :format           => "%s:\n%s",
    :field_format     => "%30s = %s",
    :separator        => "\n",
    :field_name_meth  => :display_name,
    :include_rest     => true
  }
  
  # A standard inspect method which does not add newlines.
  def inspect(opts = DEFAULT_INSPECT_OPTS)
    field_format = opts[:field_format]
    field_name_meth = opts[:field_name_meth]
    
    fields_for_inspect = fields.select {|field| field.inspectable?}
    if opts[:include_rest] and (rest_field = self.class.rest_field)
      fields_for_inspect << rest_field
    end
    
    ary = fields_for_inspect.map do |field|
      field_format %
       [field.send(field_name_meth),
        field.inspect_in_object(self, opts)]
    end
        
    body = ary.join(opts[:separator])
    
    opts[:format] % [self.class, body]
  end
  
  # A more visually appealing inspect method that puts each field/value on
  # a separate line. Very useful when output is scrolling by on a screen.
  #
  # (This is actually a convenience method to call #inspect with the
  # DETAILED_INSPECT_OPTS opts.)
  def inspect_detailed
    inspect(DETAILED_INSPECT_OPTS)
  end

  # ------------------------
  # :section: field declaration methods
  #
  # ------------------------
  
  # Define accessors for a variable length substring from the end of
  # the defined fields to the end of the BitStruct. The _rest_ may behave as
  # a String or as some other String or BitStruct subclass.
  #
  # This does not add a field, which is useful because a superclass can have
  # a rest method which accesses subclass data. In particular, #rest does
  # not affect the #round_byte_length class method. Of course, any data
  # in rest does add to the #length of the BitStruct, calculated as a string.
  # Also, _rest_ is not inherited.
  #
  # The +ary+ argument(s) work as follows:
  #
  # If a class is provided, use it for the Field class (String by default).
  # If a string is provided, use it for the display_name (+name+ by default).
  # If a hash is provided, use it for options.
  #
  # *Warning*: the rest reader method returns a copy of the field, so
  # accessors on that returned value do not affect the original rest field. 
  #
  def self.rest(name, *ary)
    if @rest_field
      raise ArgumentError, "Duplicate rest field: #{name.inspect}."
    end
    
    opts = parse_options(ary, name, String)
    offset = round_byte_length
    byte_range = offset..-1
    class_eval do
      field_class = opts[:field_class]
      define_method name do ||
        field_class.new(self[byte_range])
      end

      define_method "#{name}=" do |val|
        self[byte_range] = val
      end
      
      @rest_field = Field.new(offset, -1, name, {
        :display_name => opts[:display_name],
        :rest_class => field_class
      })
    end
  end
  
  # Not included with the other fields, but accessible separately.
  def self.rest_field; @rest_field; end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for fixed length binary strings of characters.
  # Declared with BitStruct.char.
  class CharField < Field
    #def self.default
    #  don't define this, since it must specify N nulls and we don't know N
    #end
    
    # Used in describe.
    def self.class_name
      @class_name ||= "char"
    end

    def add_accessors_to(cl, attr = name) # :nodoc:
      unless offset % 8 == 0
        raise ArgumentError,
          "Bad offset, #{offset}, for #{self.class} #{name}." +
          " Must be multiple of 8."
      end
      
      unless length % 8 == 0
        raise ArgumentError,
          "Bad length, #{length}, for #{self.class} #{name}." +
          " Must be multiple of 8."
      end
      
      offset_byte = offset / 8
      length_byte = length / 8
      last_byte = offset_byte + length_byte - 1
      byte_range = offset_byte..last_byte
      val_byte_range = 0..length_byte-1

      cl.class_eval do
        define_method attr do ||
          self[byte_range].to_s
        end

        define_method "#{attr}=" do |val|
          val = val.to_s
          if val.length < length_byte
            val += "\0" * (length_byte - val.length)
          end
          self[byte_range] = val[val_byte_range]
        end
      end
    end
  end
  
  class << self
    # Define a char string field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits). Trailing nulls _are_
    # considered part of the string.
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    # Note that the accessors have COPY semantics, not reference.
    #
    def char(name, length, *rest)
      opts = parse_options(rest, name, CharField)
      add_field(name, length, opts)
    end
    alias string char
  end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for floats (single and double precision) in network order.
  # Declared with BitStruct.float.
  class FloatField < Field
    # Used in describe.
    def self.class_name
      @class_name ||= "float"
    end
    
    def add_accessors_to(cl, attr = name) # :nodoc:
      unless offset % 8 == 0
        raise ArgumentError,
          "Bad offset, #{offset}, for #{self.class} #{name}." +
          " Must be multiple of 8."
      end
      
      unless length == 32 or length == 64
        raise ArgumentError,
          "Bad length, #{length}, for #{self.class} #{name}." +
          " Must be 32 or 64."
      end
      
      offset_byte = offset / 8
      length_byte = length / 8
      last_byte = offset_byte + length_byte - 1
      byte_range = offset_byte..last_byte

      endian = (options[:endian] || options["endian"]).to_s
      case endian
      when "native"
        ctl = case length
          when 32; "f"
          when 64; "d"
        end
      when "little"
        ctl = case length
          when 32; "e"
          when 64; "E"
        end
      when "network", "big", ""
        ctl = case length
          when 32; "g"
          when 64; "G"
        end
      else
        raise ArgumentError,
          "Unrecognized endian option: #{endian.inspect}"
      end
      
      cl.class_eval do
        define_method attr do ||
          self[byte_range].unpack(ctl).first
        end

        define_method "#{attr}=" do |val|
          self[byte_range] = [val].pack(ctl)
        end
      end
    end
  end

  class << self
    # Define a floating point field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits).
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    # The <tt>:endian => :native</tt> option overrides the default of
    # <tt>:network</tt> byte ordering, in favor of native byte ordering. Also
    # permitted are <tt>:big</tt> (same as <tt>:network</tt>) and
    # <tt>:little</tt>.
    #
    def float name, length, *rest
      opts = parse_options(rest, name, FloatField)
      add_field(name, length, opts)
    end
  end
end
#require 'bit-struct/char-field'

class BitStruct
  # Class for char fields that can be accessed with values like
  # "xxx.xxx.xxx.xxx", where each xxx is up to 3 decimal digits representing a
  # single octet. The original string-based accessors are still available with
  # the <tt>_chars</tt> suffix.
  # 
  # Declared with BitStruct.octets.
  class OctetField < BitStruct::CharField
    # Used in describe.
    def self.class_name
      @class_name ||= "octets"
    end
    
    SEPARATOR = "."
    FORMAT    = "%d"
    BASE      = 10

    def add_accessors_to(cl, attr = name) # :nodoc:
      attr_chars = "#{attr}_chars"
      super(cl, attr_chars)
      sep   = self.class::SEPARATOR
      base  = self.class::BASE
      fmt   = self.class::FORMAT
      
      cl.class_eval do
        define_method attr do ||
          ary = []
          send(attr_chars).each_byte do  |c|
            ary << fmt % c
          end
          ary.join(sep)
        end
        
        old_writer = "#{attr_chars}="

        define_method "#{attr}=" do |val|
          data = val.split(sep).map{|s|s.to_i(base)}.pack("c*")
          send(old_writer, data)
        end
      end
    end
  end
  
  class << self
    # Define an octet string field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits). Trailing nulls are
    # not considered part of the string. The field is accessed using
    # period-separated decimal digits.
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    def octets(name, length, *rest)
      opts = parse_options(rest, name, OctetField)
      add_field(name, length, opts)
    end
  end
end
#require 'bit-struct/char-field'

class BitStruct
  # Class for char fields that can be accessed with values like
  # "xx:xx:xx:xx", where each xx is up to 2 hex digits representing a
  # single octet. The original string-based accessors are still available with
  # the <tt>_chars</tt> suffix.
  # 
  # Declared with BitStruct.hex_octets.
  class HexOctetField < BitStruct::OctetField
    # Used in describe.
    def self.class_name
      @class_name ||= "hex_octets"
    end
    
    SEPARATOR = ":"
    FORMAT    = "%02x"
    BASE      = 16
  end
  
  class << self
    # Define an octet string field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits). Trailing nulls are
    # not considered part of the string. The field is accessed using
    # period-separated hex digits.
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    def hex_octets(name, length, *rest)
      opts = parse_options(rest, name, HexOctetField)
      add_field(name, length, opts)
    end
  end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for nesting a BitStruct as a field within another BitStruct.
  # Declared with BitStruct.nest.
  class NestedField < Field
    def initialize(*args)
      super
    end
    
    # Used in describe.
    def self.class_name
      @class_name ||= "nest"
    end
    
    def class_name
      @class_name ||= nested_class.name[/\w+$/]
    end
    
    def nested_class
      @nested_class ||= options[:nested_class] || options["nested_class"]
    end

    def describe opts
      if opts[:expand]
        opts = opts.dup
        opts[:byte_offset] = offset / 8
        opts[:omit_header] = opts[:omit_footer] = true
        nested_class.describe(nil, opts) {|desc| yield desc}
      else
        super
      end
    end

    def add_accessors_to(cl, attr = name) # :nodoc:
      unless offset % 8 == 0
        raise ArgumentError,
          "Bad offset, #{offset}, for nested field #{name}." +
          " Must be multiple of 8."
      end
      
      unless length % 8 == 0
        raise ArgumentError,
          "Bad length, #{length}, for nested field #{name}." +
          " Must be multiple of 8."
      end
      
      offset_byte = offset / 8
      length_byte = length / 8
      last_byte = offset_byte + length_byte - 1
      byte_range = offset_byte..last_byte
      val_byte_range = 0..length_byte-1

      nc = nested_class
      
      cl.class_eval do
        define_method attr do ||
          nc.new(self[byte_range])
        end

        define_method "#{attr}=" do |val|
          if val.length != length_byte
            raise ArgumentError, "Size mismatch in nested struct assignment " +
              "to #{attr} with value #{val.inspect}"
          end
          
          if val.class != nc
            warn "Type mismatch in nested struct assignment " +
              "to #{attr} with value #{val.inspect}"
          end
          
          self[byte_range] = val[val_byte_range]
        end
      end
    end
  end
  
  class << self
    # Define a nested field in the current subclass of BitStruct,
    # with the given _name_ and _nested_class_. Length is determined from
    # _nested_class_.
    #
    # In _rest_:
    #
    # If a class is provided, use it for the Field class (i.e. <=NestedField).
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    # WARNING: the accessors have COPY semantics, not reference. When you call a
    # reader method to get the nested structure, you get a *copy* of that data.
    #
    # For example:
    #
    #   class Sub < BitStruct
    #     unsigned :x,    8
    #   end
    #
    #   class A < BitStruct
    #     nest    :n,  Sub
    #   end
    #
    #   a = A.new
    #
    #   p a  # ==> #<A n=#<Sub x=0>>
    #
    #   # This fails to set x in a.
    #   a.n.x = 3
    #   p a  # ==> #<A n=#<Sub x=0>>
    #
    #   # This works
    #   n = a.n
    #   n.x = 3
    #   a.n = n
    #   p a  # ==> #<A n=#<Sub x=3>>
    # 
    def nest(name, nested_class, *rest)
      opts = parse_options(rest, name, NestedField)
      opts[:default] ||= nested_class.initial_value.dup
      opts[:nested_class] = nested_class
      field = add_field(name, nested_class.bit_length, opts)
      field
    end
    alias struct nest
  end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for fixed length padding.
  class PadField < Field
    # Used in describe.
    def self.class_name
      @class_name ||= "padding"
    end

    def add_accessors_to(cl, attr = name) # :nodoc:
      # No accessors for padding.
    end

    def inspectable?; false; end
  end
  
  class << self
    # Define a padding field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits).
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    def pad(name, length, *rest)
      opts = parse_options(rest, name, PadField)
      add_field(name, length, opts)
    end
    alias padding pad
  end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for signed integers in network order, 1-16 bits, or 8n bits.
  # Declared with BitStruct.signed.
  class SignedField < Field
    # Used in describe.
    def self.class_name
      @class_name ||= "signed"
    end
    
    def add_accessors_to(cl, attr = name) # :nodoc:
      offset_byte = offset / 8
      offset_bit = offset % 8
      
      length_bit = offset_bit + length
      length_byte = (length_bit/8.0).ceil
      last_byte = offset_byte + length_byte - 1
      max = 2**length-1
      mid = 2**(length-1)
      max_unsigned = 2**length
      to_signed = proc {|n| (n>=mid) ? n - max_unsigned : n}
#      to_signed = proc {|n| (n>=mid) ? -((n ^ max) + 1) : n}
      
      divisor = options[:fixed] || options["fixed"]
      divisor_f = divisor && divisor.to_f
#      if divisor and not divisor.is_a? Fixnum
#        raise ArgumentError, "fixed-point divisor must be a fixnum"
#      end

      endian = (options[:endian] || options["endian"]).to_s
      case endian
      when "native"
        ctl = length_byte <= 2 ? "s" : "l"
        if length == 16 or length == 32
          to_signed = proc {|n| n}
          # with pack support, to_signed can be replaced with no-op
        end
      when "little"
        ctl = length_byte <= 2 ? "v" : "V"
      when "network", "big", ""
        ctl = length_byte <= 2 ? "n" : "N"
      else
        raise ArgumentError,
          "Unrecognized endian option: #{endian.inspect}"
      end
      
      data_is_big_endian =
        ([1234].pack(ctl) == [1234].pack(length_byte <= 2 ? "n" : "N"))
      
      if length_byte == 1
        rest = 8 - length_bit
        mask  = ["0"*offset_bit + "1"*length + "0"*rest].pack("B8")[0]
        mask2 = ["1"*offset_bit + "0"*length + "1"*rest].pack("B8")[0]
        
        cl.class_eval do
          if divisor
            define_method attr do ||
              to_signed[(self[offset_byte] & mask) >> rest] / divisor_f
            end

            define_method "#{attr}=" do |val|
              val = (val * divisor).round
              self[offset_byte] =
                (self[offset_byte] & mask2) | ((val<<rest) & mask)
            end

          else
            define_method attr do ||
              to_signed[(self[offset_byte] & mask) >> rest]
            end

            define_method "#{attr}=" do |val|
              self[offset_byte] =
                (self[offset_byte] & mask2) | ((val<<rest) & mask)
            end
          end
        end
      
      elsif offset_bit == 0 and length % 8 == 0
        field_length = length
        byte_range = offset_byte..last_byte
        
        cl.class_eval do
          case field_length
          when 8
            if divisor
              define_method attr do ||
                to_signed[self[offset_byte]] / divisor_f
              end

              define_method "#{attr}=" do |val|
                val = (val * divisor).round
                self[offset_byte] = val
              end
          
            else
              define_method attr do ||
                to_signed[self[offset_byte]]
              end

              define_method "#{attr}=" do |val|
                self[offset_byte] = val
              end
            end
        
          when 16, 32
            if divisor
              define_method attr do ||
                to_signed[self[byte_range].unpack(ctl).first] / divisor_f
              end

              define_method "#{attr}=" do |val|
                val = (val * divisor).round
                self[byte_range] = [val].pack(ctl)
              end
            
            else
              define_method attr do ||
                to_signed[self[byte_range].unpack(ctl).first]
              end

              define_method "#{attr}=" do |val|
                self[byte_range] = [val].pack(ctl)
              end
            end
          
          else
            reader_helper = proc do |substr|
              bytes = substr.unpack("C*")
              bytes.reverse! unless data_is_big_endian
              bytes.inject do |sum, byte|
                (sum << 8) + byte
              end
            end
            
            writer_helper = proc do |val|
              bytes = []
              val += max_unsigned if val < 0
              while val > 0
                bytes.push val % 256
                val = val >> 8
              end
              if bytes.length < length_byte
                bytes.concat [0] * (length_byte - bytes.length)
              end

              bytes.reverse! if data_is_big_endian
              bytes.pack("C*")
            end
            
            if divisor
              define_method attr do ||
                to_signed[reader_helper[self[byte_range]] / divisor_f]
              end
              
              define_method "#{attr}=" do |val|
                self[byte_range] = writer_helper[(val * divisor).round]
              end
            
            else
              define_method attr do ||
                to_signed[reader_helper[self[byte_range]]]
              end
              
              define_method "#{attr}=" do |val|
                self[byte_range] = writer_helper[val]
              end
            end
          end
        end

      elsif length_byte == 2 # unaligned field that fits within two whole bytes
        byte_range = offset_byte..last_byte
        rest = 16 - length_bit
        
        mask  = ["0"*offset_bit + "1"*length + "0"*rest]
        mask = mask.pack("B16").unpack(ctl).first
        
        mask2 = ["1"*offset_bit + "0"*length + "1"*rest]
        mask2 = mask2.pack("B16").unpack(ctl).first

        cl.class_eval do
          if divisor
            define_method attr do ||
              to_signed[(self[byte_range].unpack(ctl).first & mask) >> rest] /
                 divisor_f
            end

            define_method "#{attr}=" do |val|
              val = (val * divisor).round
              x = (self[byte_range].unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)
            end

          else
            define_method attr do ||
              to_signed[(self[byte_range].unpack(ctl).first & mask) >> rest]
            end

            define_method "#{attr}=" do |val|
              x = (self[byte_range].unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)
            end
          end
        end
      
      elsif length_byte == 3 # unaligned field that fits within 3 whole bytes
        byte_range = offset_byte..last_byte
        rest = 32 - length_bit
        
        mask  = ["0"*offset_bit + "1"*length + "0"*rest]
        mask = mask.pack("B32").unpack(ctl).first
        
        mask2 = ["1"*offset_bit + "0"*length + "1"*rest]
        mask2 = mask2.pack("B32").unpack(ctl).first

        cl.class_eval do
          if divisor
            define_method attr do ||
              bytes = self[byte_range]
              bytes << 0
              to_signed[((bytes.unpack(ctl).first & mask) >> rest)] /
                 divisor_f
            end

            define_method "#{attr}=" do |val|
              val = (val * divisor).round
              bytes = self[byte_range]
              bytes << 0
              x = (bytes.unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)[0..2]
            end

          else
            define_method attr do ||
              bytes = self[byte_range]
              bytes << 0
              to_signed[(bytes.unpack(ctl).first & mask) >> rest]
            end

            define_method "#{attr}=" do |val|
              bytes = self[byte_range]
              bytes << 0
              x = (bytes.unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)[0..2]
            end
          end
        end
      
      else
        raise "unsupported: #{inspect}"
      end
    end
  end

  class << self
    # Define a signed integer field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits).
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    # SignedField adds the <tt>:fixed => divisor</tt> option, which specifies
    # that the internally stored value is interpreted as a fixed point real
    # number with the specified +divisor+.
    #
    # The <tt>:endian => :native</tt> option overrides the default of
    # <tt>:network</tt> byte ordering, in favor of native byte ordering. Also
    # permitted are <tt>:big</tt> (same as <tt>:network</tt>) and
    # <tt>:little</tt>.
    #
    def signed name, length, *rest
      opts = parse_options(rest, name, SignedField)
      add_field(name, length, opts)
    end
  end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for null-terminated printable text strings.
  # Declared with BitStruct.text.
  class TextField < Field
    # Used in describe.
    def self.class_name
      @class_name ||= "text"
    end

    def add_accessors_to(cl, attr = name) # :nodoc:
      unless offset % 8 == 0
        raise ArgumentError,
          "Bad offset, #{offset}, for #{self.class} #{name}." +
          " Must be multiple of 8."
      end
      
      unless length % 8 == 0
        raise ArgumentError,
          "Bad length, #{length}, for #{self.class} #{name}." +
          " Must be multiple of 8."
      end
      
      offset_byte = offset / 8
      length_byte = length / 8
      last_byte = offset_byte + length_byte - 1
      byte_range = offset_byte..last_byte
      val_byte_range = 0..length_byte-1

      cl.class_eval do
        define_method attr do ||
          self[byte_range].sub(/\0*$/, "").to_s
        end

        define_method "#{attr}=" do |val|
          val = val.to_s
          if val.length < length_byte
            val += "\0" * (length_byte - val.length)
          end
          self[byte_range] = val[val_byte_range]
        end
      end
    end
  end
  
  class << self
    # Define a printable text string field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits). Trailing nulls are
    # _not_ considered part of the string.
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    # Note that the accessors have COPY semantics, not reference.
    #
    def text(name, length, *rest)
      opts = parse_options(rest, name, TextField)
      add_field(name, length, opts)
    end
  end
end
#require 'bit-struct/bit-struct'

class BitStruct
  # Class for unsigned integers in network order, 1-16 bits, or 8n bits.
  # Declared with BitStruct.unsigned.
  class UnsignedField < Field
    # Used in describe.
    def self.class_name
      @class_name ||= "unsigned"
    end
    
    def add_accessors_to(cl, attr = name) # :nodoc:
      offset_byte = offset / 8
      offset_bit = offset % 8
      
      length_bit = offset_bit + length
      length_byte = (length_bit/8.0).ceil
      last_byte = offset_byte + length_byte - 1
      
      divisor = options[:fixed] || options["fixed"]
      divisor_f = divisor && divisor.to_f
#      if divisor and not divisor.is_a? Fixnum
#        raise ArgumentError, "fixed-point divisor must be a fixnum"
#      end
      
      endian = (options[:endian] || options["endian"]).to_s
      case endian
      when "native"
        ctl = length_byte <= 2 ? "S" : "L"
      when "little"
        ctl = length_byte <= 2 ? "v" : "V"
      when "network", "big", ""
        ctl = length_byte <= 2 ? "n" : "N"
      else
        raise ArgumentError,
          "Unrecognized endian option: #{endian.inspect}"
      end
      
      data_is_big_endian =
        ([1234].pack(ctl) == [1234].pack(length_byte <= 2 ? "n" : "N"))
      
      if length_byte == 1
        rest = 8 - length_bit
        mask  = ["0"*offset_bit + "1"*length + "0"*rest].pack("B8")[0]
        mask2 = ["1"*offset_bit + "0"*length + "1"*rest].pack("B8")[0]
        
        cl.class_eval do
          if divisor
            define_method attr do ||
              ((self[offset_byte] & mask) >> rest) / divisor_f
            end

            define_method "#{attr}=" do |val|
              val = (val * divisor).round
              self[offset_byte] =
                (self[offset_byte] & mask2) | ((val<<rest) & mask)
            end

          else
            define_method attr do ||
              (self[offset_byte] & mask) >> rest
            end

            define_method "#{attr}=" do |val|
              self[offset_byte] =
                (self[offset_byte] & mask2) | ((val<<rest) & mask)
            end
          end
        end
      
      elsif offset_bit == 0 and length % 8 == 0
        field_length = length
        byte_range = offset_byte..last_byte
        
        cl.class_eval do
          case field_length
          when 8
            if divisor
              define_method attr do ||
                self[offset_byte] / divisor_f
              end

              define_method "#{attr}=" do |val|
                val = (val * divisor).round
                self[offset_byte] = val
              end
          
            else
              define_method attr do ||
                self[offset_byte]
              end

              define_method "#{attr}=" do |val|
                self[offset_byte] = val
              end
            end
        
          when 16, 32
            if divisor
              define_method attr do ||
                self[byte_range].unpack(ctl).first / divisor_f
              end

              define_method "#{attr}=" do |val|
                val = (val * divisor).round
                self[byte_range] = [val].pack(ctl)
              end
            
            else
              define_method attr do ||
                self[byte_range].unpack(ctl).first
              end

              define_method "#{attr}=" do |val|
                self[byte_range] = [val].pack(ctl)
              end
            end
          
          else
            reader_helper = proc do |substr|
              bytes = substr.unpack("C*")
              bytes.reverse! unless data_is_big_endian
              bytes.inject do |sum, byte|
                (sum << 8) + byte
              end
            end
            
            writer_helper = proc do |val|
              bytes = []
              while val > 0
                bytes.push val % 256
                val = val >> 8
              end
              if bytes.length < length_byte
                bytes.concat [0] * (length_byte - bytes.length)
              end

              bytes.reverse! if data_is_big_endian
              bytes.pack("C*")
            end
            
            if divisor
              define_method attr do ||
                reader_helper[self[byte_range]] / divisor_f
              end
              
              define_method "#{attr}=" do |val|
                self[byte_range] = writer_helper[(val * divisor).round]
              end
            
            else
              define_method attr do ||
                reader_helper[self[byte_range]]
              end
              
              define_method "#{attr}=" do |val|
                self[byte_range] = writer_helper[val]
              end
            end
          end
        end

      elsif length_byte == 2 # unaligned field that fits within two whole bytes
        byte_range = offset_byte..last_byte
        rest = 16 - length_bit
        
        mask  = ["0"*offset_bit + "1"*length + "0"*rest]
        mask = mask.pack("B16").unpack(ctl).first
        
        mask2 = ["1"*offset_bit + "0"*length + "1"*rest]
        mask2 = mask2.pack("B16").unpack(ctl).first

        cl.class_eval do
          if divisor
            define_method attr do ||
              ((self[byte_range].unpack(ctl).first & mask) >> rest) /
                 divisor_f
            end

            define_method "#{attr}=" do |val|
              val = (val * divisor).round
              x = (self[byte_range].unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)
            end

          else
            define_method attr do ||
              (self[byte_range].unpack(ctl).first & mask) >> rest
            end

            define_method "#{attr}=" do |val|
              x = (self[byte_range].unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)
            end
          end
        end
      
      elsif length_byte == 3 # unaligned field that fits within 3 whole bytes
        byte_range = offset_byte..last_byte
        rest = 32 - length_bit
        
        mask  = ["0"*offset_bit + "1"*length + "0"*rest]
        mask = mask.pack("B32").unpack(ctl).first
        
        mask2 = ["1"*offset_bit + "0"*length + "1"*rest]
        mask2 = mask2.pack("B32").unpack(ctl).first

        cl.class_eval do
          if divisor
            define_method attr do ||
              bytes = self[byte_range]
              bytes << 0
              ((bytes.unpack(ctl).first & mask) >> rest) /
                 divisor_f
            end

            define_method "#{attr}=" do |val|
              val = (val * divisor).round
              bytes = self[byte_range]
              bytes << 0
              x = (bytes.unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)[0..2]
            end

          else
            define_method attr do ||
              bytes = self[byte_range]
              bytes << 0
              (bytes.unpack(ctl).first & mask) >> rest
            end

            define_method "#{attr}=" do |val|
              bytes = self[byte_range]
              bytes << 0
              x = (bytes.unpack(ctl).first & mask2) |
                ((val<<rest) & mask)
              self[byte_range] = [x].pack(ctl)[0..2]
            end
          end
        end
      
      else
        raise "unsupported: #{inspect}"
      end
    end
  end

  class << self
    # Define a unsigned integer field in the current subclass of BitStruct,
    # with the given _name_ and _length_ (in bits).
    #
    # If a class is provided, use it for the Field class.
    # If a string is provided, use it for the display_name.
    # If a hash is provided, use it for options.
    #
    # UnsignedField adds the <tt>:fixed => divisor</tt> option, which specifies
    # that the internally stored value is interpreted as a fixed point real
    # number with the specified +divisor+.
    #
    # The <tt>:endian => :native</tt> option overrides the default of
    # <tt>:network</tt> byte ordering, in favor of native byte ordering. Also
    # permitted are <tt>:big</tt> (same as <tt>:network</tt>) and
    # <tt>:little</tt>.
    #
    def unsigned name, length, *rest
      opts = parse_options(rest, name, UnsignedField)
      add_field(name, length, opts)
    end
  end
end
#require 'bit-struct/bit-struct'
require 'yaml'

class BitStruct
  if RUBY_VERSION == "1.8.2"
    def is_complex_yaml? # :nodoc:
      true
    end

    YAML.add_ruby_type(/^bitstruct/) do |type, val|
      subtype, subclass = YAML.read_type_class(type, Object)
      subclass.new(val)
    end

    def to_yaml_type # :nodoc:
      "!ruby/bitstruct:#{self.class}"
    end

    def to_yaml( opts = {} ) # :nodoc:
      opts[:DocType] = self.class if Hash === opts
      YAML.quick_emit(self.object_id, opts) do |out|
        out.map(to_yaml_type) do |map|
          fields.each do |field|
            fn = field.name
            map.add(fn, send(fn))
          end
        end
      end
    end

  else
    yaml_as "tag:path.berkeley.edu,2006:bitstruct"

    def to_yaml_properties # :nodoc:
      yaml_fields = fields.select {|field| field.inspectable?}
      props = yaml_fields.map {|f| f.name.to_s}
      if (rest_field = self.class.rest_field)
        props << rest_field.name.to_s
      end
      props
    end

    # Return YAML representation of the BitStruct.
    def to_yaml( opts = {} )
      YAML::quick_emit( object_id, opts ) do |out|
        out.map( taguri, to_yaml_style ) do |map|
          to_yaml_properties.each do |m|
            map.add( m, send( m ) )
          end
        end
      end
    end

    def self.yaml_new( klass, tag, val ) # :nodoc:
      unless Hash === val
        raise YAML::TypeError, "Invalid BitStruct: " + val.inspect
      end

      bitstruct_name, bitstruct_type = YAML.read_type_class( tag, BitStruct )

      st = bitstruct_type.new

      val.each do |k,v|
        st.send( "#{k}=", v )
      end

      st
    end
  end
end

#!/usr/bin/env ruby
#
#  Copyright (C) 2008 Elan Feingold (elan at bluemandrill dot com)
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with GNU Make; see the file COPYING.  If not, write to
#  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
#  http://www.gnu.org/copyleft/gpl.html
#
#

#require 'bit-struct'

# For example, "_calloc" => "_wrap_calloc"
prefix = '___wrap_'
mappings = {
#'calloc' => true,
'clearerr' => true,
'close' => true,
'fclose' => true,
'fdopen' => true,
'feof' => true,
'ferror' => true,
'fflush' => true,
'fgetc' => true,
'fgetpos' => true,
'fgets' => true,
'fileno' => true,
'flockfile' => true,
'fopen' => true,
'fopen64' => true,
'fprintf' => true,
'fputc' => true,
'fputs' => true,
'fread' => true,
#'free' => true,
'freopen' => true,
'fseek' => true,
'fsetpos' => true,
'fstatvfs64' => true,
'ftell' => true,
'ftrylockfile' => true,
'funlockfile' => true,
'fwrite' => true,
'getc_unlocked' => true,
'ioctl' => true,
'lseek' => true,
'lseek64' => true,
#'malloc' => true,
'open' => true,
'open64' => true,
'popen' => true,
'printf' => true,
'read' => true,
'opendir' => true,
'readdir' => true,
'closedir' => true,
'rewinddir' => true,
#'realloc' => true,
'rewind' => true,
'stat' => true,
'fstat' => true,
'ungetc' => true,
'vfprintf' => true,
'write' => true,
'putc' => '___wrap__IO_putc',
'getc' => '___wrap__IO_getc',
'getc_unlocked' => '___wrap__IO_getc_unlocked'
}

prefix_python = '___py_wrap_'
mappings_python = {
  'getcwd' => true, 'chdir' => true, 'access' => true, 'unlink' => true, 'chmod' => true, 
  'rmdir' => true, 'utime' => true, 'rename' => true, 'mkdir' => true,  
  'dlopen' => true, 'dlclose' => true, 'dlsym' => true,
  'lstat' => true
}

LC_SYMTAB = 0x02
LC_SEGMENT = 0x01

class MachHeader < BitStruct
  hex_octets  :magic,       32, "Magic Number"
  unsigned    :cputype,     32, "CPU Type", :endian => :native
  unsigned    :cpusubtype,  32, "CPU Subtype", :endian => :native
  unsigned    :filetype,    32, "File Type", :endian => :native
  unsigned    :ncmds,       32, "Number of commands", :endian => :native
  unsigned    :sizeofcmds,  32, "Size of commands", :endian => :native
  unsigned    :flags,       32, "Flags", :endian => :native
  rest        :data,            "Data"
end

class LoadCommand < BitStruct
  unsigned    :cmd,         32, "Command", :endian => :native
  unsigned    :cmdsize,     32, "Command Size", :endian => :native
end

class SymtabCommand < BitStruct
  unsigned    :cmd,         32, "Command", :endian => :native
  unsigned    :cmdsize,     32, "Command Size", :endian => :native
  unsigned    :symoff,      32, "Symbol Offset", :endian => :native
  unsigned    :nsyms,       32, "Number of Symbols", :endian => :native
  unsigned    :stroff,      32, "String table offset", :endian => :native
  unsigned    :strsize,     32, "Size of string table", :endian => :native
end

class SegmentCommand < BitStruct
  unsigned    :cmd,         32, "Command", :endian => :native
  unsigned    :cmdsize,     32, "Command Size", :endian => :native
  char        :segname,    16*8, "Segment name", :endian => :native
  unsigned    :vmaddr,      32, "VM Adddress", :endian => :native
  unsigned    :vmsize,      32, "VM Size", :endian => :native
  unsigned    :fileoff,     32, "File Offset", :endian => :native
  unsigned    :filesize,    32, "File Size", :endian => :native
end

class SymtabEntry < BitStruct
  unsigned  :strtableoffset, 32, "String table offset", :endian => :native
  unsigned  :debuggingEntry,  3, "Debugging entry", :endian => :native
  unsigned  :privateExternal, 1, "Is Private Enternal", :endian => :native
  unsigned  :type,            3, "Type bits", :endian => :native
  unsigned  :external,        1, "External symbol", :endian => :native
  unsigned  :sectionNumber,   8, "Section number", :endian => :native
  unsigned  :description,    16, "Description", :endian => :native
  unsigned  :value,          32, "Value", :endian => :native
end

# Select which mapping to use.
if ARGV.size() > 1 and ARGV[1].index('libpython') == 0
  puts "Using Python mappings."
  mappings = mappings_python
  #prefix = prefix_python
end

data = open(ARGV[0]).read
puts "Input file was #{data.length} bytes long."

# Parse the header.
header = MachHeader.new(data)
sym_cmd = nil

# String table.
string_table = nil
string_table_offset = nil
string_table_map = {}
offset_map = {}

# Symbol table.
symbol_table = nil
symbol_table_offset = nil
symbols = []

# Link segment.
link_cmd = nil
link_cmd_offset = nil

# Walk through all the commands.
offset = data.size - header.data.size
header.ncmds.times do |i|
  load_cmd = LoadCommand.new(data[offset..-1])
  
  if load_cmd.cmd == LC_SEGMENT
    seg_cmd = SegmentCommand.new(data[offset..-1])
    if seg_cmd.segname.index('__LINKEDIT') == 0
      puts "Found LINKEDIT segment at offset #{offset}"
      link_cmd = seg_cmd
      link_cmd_offset = offset
    end
  end
  
  if load_cmd.cmd == LC_SYMTAB
    # Parse the symbol table command.
    sym_cmd = SymtabCommand.new(data[offset..-1])
    symbol_table_offset = offset
    
    # Parse the string table, store with offsets.
    string_table_offset = sym_cmd.stroff
    string_table = data[sym_cmd.stroff..sym_cmd.stroff+sym_cmd.strsize-1]
    i = 0
    string_table.split("\x00", -1).each do |s|
      string_table_map[i] = s
      i += s.length + 1
    end
    
    # Parse the symbol table.
    symbol_table = data[sym_cmd.symoff..-1]
    i = 0
    puts "Symbol table has #{sym_cmd.nsyms} symbols."
    sym_cmd.nsyms.times do |n|
      symbols << SymtabEntry.new(symbol_table[i..i+11])
      i += 12
    end
    
    # Now go through and make renames to the symbols.
    size_diff = 0
    
    string_table_map.keys.sort.each do |i|
      orig_sym = string_table_map[i]
      
      # Store the offset mapping.
      offset_map[i] = (i + size_diff)
      
      if orig_sym.length > 1
        sym = orig_sym[1..-1].gsub('$UNIX2003','')
        if mappings.has_key?(sym)
          if mappings[sym] != true
            string_table_map[i] = mappings[sym]
          else
            string_table_map[i] = "#{prefix}#{sym}"
          end
          puts "   - Mapping: #{orig_sym} to #{string_table_map[i]} (offset #{i} -> #{i + size_diff})"
                          
          # Accumulate the offset difference.
          size_diff += string_table_map[i].length - orig_sym.length
        end
      end
    end
  end
  
  offset += load_cmd.cmdsize
end

# OK, now lets rewrite the symbol table. Offsets may have changed, but the size doesn't.
new_symbol_table = ''
i = 0
symbols.each do |symbol|
  puts "  - Mapped #{i} symbols..." if i % 10000 == 0 and i > 0
  symbol.strtableoffset = offset_map[symbol.strtableoffset] if symbol.strtableoffset > 1
  new_symbol_table << symbol
  i += 1
end

# OK, now lets rewrite the string table. The size will be different if mappings have occurred.
new_string_table = string_table_map.keys.sort.collect { |i| string_table_map[i] }.join("\x00")

# Next, modify the LC_SYMTAB header.
size_diff = new_string_table.length - sym_cmd.strsize
sym_cmd.strsize = new_string_table.length

# Lastly, modify the LINKEDIT segment if it exists.
if link_cmd
  puts "Size changed by #{size_diff} bytes, rewriting LINKEDIT segment."
  link_cmd.filesize += size_diff
  SegmentCommand.round_byte_length.times { |i| data[link_cmd_offset + i] = link_cmd[i] }
end

# Create the new file in memory. First, copy the new symbol table header into place.
24.times { |i| data[symbol_table_offset + i] = sym_cmd[i] }

# Now copy the new symbol table.
new_symbol_table.length.times { |i| data[sym_cmd.symoff + i] = new_symbol_table[i] }

# Finally, add the new string table.
data = data[0..string_table_offset-1] + new_string_table

puts "Output file is #{data.length} bytes long."
open("output.so", "wb").write(data)
