require "colorize"

macro enum_to_s
	{% begin %}
		case self 
			{% for val in @type.constants %}
				when {{ val.id }} then io << "{{ val.id }}"
			{% end %}
		end
	{% end %}
end

# generates an enum that maps strings to its values 
macro mapped_enum(name, map)
	enum {{ name.id }}
		{% for k,v in map %}
			{{ v }}
		{% end %}
		def self.test_s(s : String)
			{{ map }}[s]? != nil
		end
		def self.from_s(s : String)
			{{ map }}[s] 
		end
		def self.[]?(s : String) : {{ name.id }} | Nil
			{{ map }}[s]?
		end
		def to_s(io : IO)
		{% verbatim do %}
			enum_to_s
		{% end %}
		end
	end	
end

class String
	def indent
		String.build do |s|
			(lines = self.lines).each_with_index do |l,i|
				s << "    " << l << ("\n" unless i == lines.size - 1)
			end
		end
	end
end

class Token
  	enum Kind 
		Null
		EOF
		# ~ Keywords
		Group
	    Diagnostic
		Message
		Type
		Warn
		Error
		# ~ LangCodes
		EN
		EO
		JP
		# ~ Punctuation
		LBrace
		RBrace
		Colon
		Semicolon
		# ~ Dynamic
		String
		Word
  	end

	property file : Path
	property line : Int32
	property col  : Int32
	property kind : Kind = Kind::Null
	property data : (String | Nil) = nil

	def initialize(@file, @line, @col)
	end

	def location
		"#{file.colorize.cyan}:#{line}:#{col}"
	end

	def location_to_s(io : IO)
		io << location
	end

	def to_s(io : IO)
		{% begin %}
			io << location << ": " << case kind
				{% for v in Kind.constants %}
					when Kind::{{ v }} 
						"{{ v }} " 
				{% end %}
			end << data
		{% end %}
	end
end


alias DoubleInt = Tuple(Int32, Int32)

macro errs(*data)
	class Err
		enum Kind 
			{% for v in data %}
				{{ v[0].id.camelcase }}
			{% end %}
		end

		property token : Token
		property kind  : Kind
		property data  : String | DoubleInt | Nil

		def initialize(@token, @kind, @data)
		end

		{% for v in data %}    
			{% unless v[1].is_a? NilLiteral || v[1].resolve.is_a? TypeNode%}
				raise "the second argument of an error tuple must be nil or a typename"
			{% end %}
												# this is just trying to form (token [, name]) when the second argument is not nil
			def self.{{ v[0].id }}( {{ (["token", v[1]].reject(&.nil?).map {|x| x.id.downcase }).splat }} ) : Err
				e = Err.new token, Kind::{{ v[0].id.camelcase }}, nil
				{% if v[1] %}
					e.data = {{ v[1].id.downcase }}
				{% end %}
				return e
			end
		{% end %}

		def to_s(io : IO)
			io << "#{token.location}: #{"error".colorize.red}: " <<
			case kind 
			{% for v in data %}
				when Kind::{{ v[0].id.camelcase }} then {{ v[2] }}
			{% end %}
			end
		end
	end
end


errs(
	{invalid_token, String, "invalid token: #{data.as(String)}"},
	{unexpected_token, String, "expected #{data.as(String)}"},
	{unterminated_string, nil, "unterminated string"},
	{expected_group, nil, "expected a group at top level"},
	{type_already_declared, nil, "the type of this diagnostic was already declared"},
	{message_already_declared, nil, "the message of this diagnostic was already declared"},
	{missing_message, nil, "diagnostics need to declare a message"},
	{missing_type, nil, "diagnostics need to declare a type"},
	{unknown_formatting, String, "unknown formatting specifier '#{data.as(String)}'"},
	{formatting_name_empty, nil, "formatting has an empty name (eg. something like %String:% exists when it should be %String:my_name%)"},
	{dollar_not_num, nil, "$$ was used but what's inside is not a number"},
	{dollar_oob, DoubleInt, "$$ was used but the number given (#{data.as(DoubleInt)[0]}) is larger than the amount of formats given at this point #{data.as(DoubleInt)[1]}"}
)

class Lexer 
	@file : Path
	@line = 1
	@col  = 1
	@peeked_char : Char | Nil = nil
	@peeked_token : Token | Nil = nil
	@buffer : String
	@chars : Iterator(Char)

	def initialize(path : Path | String) 
		@file = Path[path]
		@buffer = File.read path
		@chars = @buffer.each_char
	end

	def next 
		unless (peeked = @peeked_char).nil?
			res = peeked
			@peeked_char = nil
			return res
		end

		n = @chars.next
		return nil if n.is_a? Iterator::Stop
		if n == '\n'
			@line += 1
			@col = 1
		else
			@col += 1
		end
		n
	end

	def peek
		if @peeked_char.nil?
			@peeked_char = self.next
		end
		@peeked_char 
	end

	def skip_whitespace 
		loop do
			n = self.peek
			return if n.nil? || !n.whitespace?
			self.next
		end
	end

	def next_token : Token | Err
		unless (peeked = @peeked_token).nil?
			res = peeked
			@peeked_token = nil
			return res
		end
	
		t = Token.new @file, @line, @col

		self.skip_whitespace

		case c = self.peek
		when nil then t.kind = Token::Kind::EOF
		when .letter?
			self.next
			word = c.to_s
			loop do 
				c = self.peek
				break if c.nil? || !c.alphanumeric? && c != '_'
				word += c
				self.next
			end
			mapping = {
				"type" => Token::Kind::Type,
				"error" => Token::Kind::Error,
				"warn" => Token::Kind::Warn,
				"message" => Token::Kind::Message,
				"en" => Token::Kind::EN,
				"eo" => Token::Kind::EO,
				"jp" => Token::Kind::JP
			}
			if k = mapping[word]?
				t.kind = k
			else
				t.kind = Token::Kind::Word
				t.data = word
			end
		when '"'
			self.next
			s = ""
			loop do
				c = self.next
				break if c == '"' || c.nil?
				s += c
			end
			if c.nil? 
				return Err.unterminated_string t
			end
			t.kind = Token::Kind::String
			t.data = s
		when '/'
			self.next
			case c = self.next
			when '/'
				loop do
					c = self.next
					break if c.nil? || c == '\n'
				end
				return self.next_token # :P
			else return Err.unexpected_token t, "expected another '/' for comment"
			end
		else 
			case c = self.next
			when '{' then t.kind = Token::Kind::LBrace
			when '}' then t.kind = Token::Kind::RBrace
			when ';' then t.kind = Token::Kind::Semicolon
			when ':' then t.kind = Token::Kind::Colon
			else
				return Err.invalid_token t, c.to_s
			end
		end
		return t
	end
end

class Diagnostic
	enum Type
		Fatal
		Error
		Warning
		Notice
		Info
		Debug
		Trace

		def to_s
			case self
			when Error then "error".colorize.red
			when Warning then "warn".colorize.yellow
			when Fatal then "fatal".colorize.red
			when Notice then "notice".colorize.blue
			when Info then "info".colorize.blue
			when Debug then "debug".colorize.green
			when Trace then "trace".colorize.cyan
			end
		end
	end
	
	# collection of strings to be output for various languages
	struct Message
		enum Lang
			English
			Japanese
			Esperanto
		end

		property \
		entries = {} of Lang => String

		def to_s
			String.build do |s|
				entries.each do |k,v|
					s << k << ": " << "\"#{v}\"" << "\n"
				end
			end
		end

		def to_s(io : IO)
			io << to_s
		end
	end

	property \
	name : String,
	type : Type,
	message : Message,
	start : Token

	def initialize(@name, @type, @message, @start)
	end

	def to_s
		String.build do |s|
			s << <<-END
				#{"diagnostic".colorize.magenta} #{name.colorize.cyan}:
				    type: #{type.to_s}
				    messages: \n
				END
			s << message.to_s.indent.indent
		end
	end

	def to_s(io : IO)
		io << to_s
	end

	struct Arg 
		enum Kind
			Str
			Path
			Id
			Token
			Type
			Num
		end

		property name : String | Nil
		property kind : Kind

		def initialize(@name, @kind)
		end

		def to_s
			String.build do |s|
				s << 
				case kind
				when Kind::Str   then "Arg(Str"
				when Kind::Path  then "Arg(Path"
				when Kind::Id    then "Arg(Id"
				when Kind::Token then "Arg(Token"
				when Kind::Type  then "Arg(Type"
				when Kind::Num   then "Arg(Num"
				end << 
				if name 
					",#{name})"
				else
					")"
		   		end
			end
		end

		def to_s(io : IO)
			io << to_s
		end
	end

	def parse_message_string(s) : Array(String | Arg | Int32) | Err
		arr = [] of String | Arg | Int32
		loop do
			case c = s.index /\$|%/ 
			when nil then arr.push(s) && break
			else
				post = s[c+1..]
				case s[c]
				when '%'
					case c2 = post.index '%'
					when nil then arr.push s[..c] 
					else
						format = post[..c2-1]
						parts = format.partition ':'
						name = nil
						if parts[1] == ":"
							if parts[2].empty?
								return Err.formatting_name_empty start
							end
							name = parts[2]
						end

						format_map = {
							"str" => Arg::Kind::Str,
							"path" => Arg::Kind::Path,
							"id" => Arg::Kind::Id,
							"token" => Arg::Kind::Token,
							"type" => Arg::Kind::Type,
							"num" => Arg::Kind::Num
						}

						unless type = format_map[parts[0]]?
							return Err.unknown_formatting start, parts[0]
						end

						arr.push s[..c-1], Arg.new(name, type)
						post = post[c2+1..]
					end
				when '$'
					case c2 = post.index '$'
					when nil then arr.push s[..c]
					else
						unless n = post[..c2-1].to_i?
							return Err.dollar_not_num start
						end
						
						if n >= arr.size
							return Err.dollar_oob start, {n, arr.size}
						end

						arr.push s[..c-1], n
						post = post[c2+1..]
					end
				end
				s = post 
			end
		end
		return arr
	end

	macro place_arg(arg)
		case {{arg}}.kind
		when Arg::Kind::Str   then ".append(diag->args[#{n_args}].string)"
		when Arg::Kind::Path  then ".path(diag->args[#{n_args}].string)"
		when Arg::Kind::Id    then ".identifier(diag->args[#{n_args}].string)"
		when Arg::Kind::Token then ".append(diag->args[#{n_args}].token)"
		when Arg::Kind::Type  then ".append(diag->args[#{n_args}].type)"
		when Arg::Kind::Num   then ".append(diag->args[#{n_args}].num)"
		else abort("unhandled arg kind")
		end + "\n"
	end

	def to_c : Tuple(String, String)
		decl = String::Builder.new
		impl = String::Builder.new
		message.entries.each do |_,m|
			if (result = parse_message_string(m)).is_a? Err 
				puts result
				return {"",""}
			end

			args = result.each.select(Arg).to_a
			argnames = [] of String

			argstr = String.build do |s|
				s << "MessageSender m"
				args.each.with_index do |arg,i|
					s << ", " << 
					case arg.kind
					when Arg::Kind::Str, Arg::Kind::Path, Arg::Kind::Id  then "String "
					when Arg::Kind::Token then "Token* "
					when Arg::Kind::Type  then "Type* "
					when Arg::Kind::Num   then "s64 "
					end 
					name = 
					case arg.name 
					when nil then "arg#{i}"
					else arg.name
					end
					argnames.push name.as(String)
					s << name
				end
			end

			decl << <<-END
			static Diag #{name}(#{argstr});
			static void #{name}(Array<Diag>& to, #{argstr});\n
			END
			
			impl << <<-END
			Diag Diag::
			#{name}(#{argstr}) {
				auto out = Diag::create(m, Kind::#{name.camelcase}, Message::Kind::#{Type.names[type.value]}, #{result.size});\n
			END

			args.zip(argnames).each.with_index do |arg,i|
				impl << "\tout.args[#{i}]." << 
				case arg[0].kind
				when Arg::Kind::Str, Arg::Kind::Path, Arg::Kind::Id then "string"
				when Arg::Kind::Token then "token"
				when Arg::Kind::Type then "type"
				when Arg::Kind::Num then "num"
				end << 
				" = #{arg[1]};\n"
			end

			impl << <<-END
				out.emit_callback = [](Diag* diag) -> Message {
					switch(language) {
						default:
						case Lang::English:
							return MessageBuilder::
								 start(diag->sender, diag->severity)\n
			END
	
			n_args = 0

			result.each do |r|
				impl << "\t" * 5 << case r
				when Arg
					s = place_arg(args[n_args])
					n_args += 1
					s
				when Int32
					place_arg(args[r])
				when String then ".append(\"#{r}\")\n"
				else
				end
			end

			impl << <<-END
								.message;
				};
				return out;
			}
			END
		end

		puts decl.to_s
		puts impl.to_s
		return {"", ""}
	end

	def to_c(io : IO)
		io << to_c
	end
end

class Parser
	@lexer : Lexer
	@curt : Token | Err

	getter diagnostics = [] of Diagnostic

	def initialize(path : String)
		@lexer = Lexer.new path
		@curt = @lexer.next_token
	end

	def next_token : Token | Err
		@curt = @lexer.next_token
		_ = @curt # makes a copy of curt (which is just a pointer) and returns it 
	end

	def start : Nil | Err
		loop do
			curt = @curt
			return curt if curt.is_a? Err
			case curt.kind
			when Token::Kind::Word
				result = diagnostic
				return result if result.is_a? Err
				@diagnostics.push result
			when Token::Kind::EOF
				return nil
			else
				return Err.unexpected_token curt, "diagnostic name"
			end
		end
	end

	def diagnostic : Diagnostic | Err
		start_token = @curt
		return start_token if start_token.is_a? Err
	
		name = start_token.data.as(String)

		t = next_token
		return t if t.is_a? Err
		unless t.kind == Token::Kind::LBrace
			return Err.unexpected_token t, "left brace '{' to start diagnostic body"
		end

		type = nil
		message = nil

		loop do
			t = next_token
			return t if t.is_a? Err
			case t.kind 
			when Token::Kind::RBrace then next_token && break
			when Token::Kind::Type
				if type
					return Err.type_already_declared t
				end
				t = next_token
				return t if t.is_a? Err
				unless t.kind == Token::Kind::Colon
					return Err.unexpected_token t, "colon ':' after key"
				end

				t = next_token
				return t if t.is_a? Err
				case t.kind
				when Token::Kind::Error
					type = Diagnostic::Type::Error
				when Token::Kind::Warn
					type = Diagnostic::Type::Warning
				else return Err.unexpected_token t, "'error' or 'warn'"
				end

				t = next_token
				return t if t.is_a? Err
				unless t.kind == Token::Kind::Semicolon
					return Err.unexpected_token t, "semicolon"
				end
			when Token::Kind::Message
				if message
					return Err.message_already_declared t
				end
				
				message = Diagnostic::Message.new

				t = next_token
				return t if t.is_a? Err
				unless t.kind == Token::Kind::Colon
					return Err.unexpected_token t, "colon ':' after 'message'"
				end

				t = next_token
				return t if t.is_a? Err
				unless t.kind == Token::Kind::LBrace
					return Err.unexpected_token t, "left brace '{' to start message body"
				end

				# map of lang code tokens to diag message langs
				# so to extend the parser you can just add more mappings here
				lang_map = {
					Token::Kind::EN => Diagnostic::Message::Lang::English,
					Token::Kind::EO => Diagnostic::Message::Lang::Esperanto,
					Token::Kind::JP => Diagnostic::Message::Lang::Japanese,
				}

				loop do
					t = next_token
					return t if t.is_a? Err
					break if t.kind == Token::Kind::RBrace 
					if lang_map.empty? 
						unless t.kind == Token::Kind::RBrace
							return Err.unexpected_token t, "right brace '}' to end message (all lang codes have been assigned)"
						end
						break
					end

					unless (lang = lang_map[t.kind]?) 
						return Err.unexpected_token t, "lang code"
					end

					lang_map.delete t.kind
					
					t = next_token
					return t if t.is_a? Err
					unless t.kind == Token::Kind::Colon
						return Err.unexpected_token t, "colon after lang code"
					end

					t = next_token
					return t if t.is_a? Err
					unless t.kind == Token::Kind::String
						return Err.unexpected_token t, "string for message lang entry"
					end

					message.entries[lang] = t.data.as(String)

					t = next_token
					return t if t.is_a? Err
					unless t.kind == Token::Kind::Semicolon
						return Err.unexpected_token t, "semicolon ';'"
					end
				end
			else return Err.unexpected_token t, "'type' or 'message' key"
			end
		end

		if message.nil? 
			return Err.missing_message start_token
		end

		if type.nil?
			return Err.missing_type start_token
		end

		return Diagnostic.new name, type, message, start_token
	end
end

p = Parser.new "src/generators/diagnostics.def"
case result = p.start
when Err then puts result
else
	p.diagnostics.each do |d|
		d.to_c
	end
end
