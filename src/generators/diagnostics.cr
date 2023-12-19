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

macro errs(*data)
	class Err
		enum Kind 
			{% for v in data %}
				{{ v[0].id.camelcase }}
			{% end %}
		end

		property token : Token
		property kind  : Kind
		property data  : String | Nil

		def initialize(@token, @kind, @data)
		end

		{% for v in data %}    
			{% unless v[1].is_a? NilLiteral || v[1].resolve.is_a? TypeNode%}
				raise "the second argument of an error tuple must be nil or a typename"
			{% end %}
												# this is just trying to form (token [, name]) when the second argument is not nil
			def self.{{ v[0].id }}( {{ *(["token", v[1]].reject(&.nil?).map {|x| x.id.downcase }) }} ) : Err
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
				"group" => Token::Kind::Group,
				"diagnostic" => Token::Kind::Diagnostic,
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
		Error
		Warning

		def to_s
			case self
			when Error then "error".colorize.red
			when Warning then "warn".colorize.yellow
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
	message : Message

	def initialize(@name, @type, @message)
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
end

class Group 
	property \
	name : String,
	diags = [] of Diagnostic
	
	def initialize(@name)
	end

	def to_s
		String.build do |s|
			s << <<-END
				#{"group".colorize.green} #{name.colorize.cyan}:\n
				END
			diags.each do |d|
				s << d.to_s.indent << "\n"
			end
		end
	end

	def to_s(io : IO)
		io << to_s
	end
end

class Parser
	@lexer : Lexer
	@curt : Token | Err

	property groups = [] of Group

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
			when Token::Kind::Group
				result = group
				return result if result.is_a? Err
				@groups.push result
			when Token::Kind::EOF
				return nil
			else
				return Err.unexpected_token curt, "group at top level"
			end
		end
	end

	def group : Group | Err
		t = next_token
		return t if t.is_a? Err
		unless t.kind == Token::Kind::Word
			return Err.unexpected_token t, "name for group"
		end
	
		g = Group.new t.data.as(String)

		t = next_token
		return t if t.is_a? Err
		unless t.kind == Token::Kind::LBrace
			return Err.unexpected_token t, "left brace '{' to start group body"
		end

		loop do
			t = next_token
			return t if t.is_a? Err
			case t.kind 
			when Token::Kind::Diagnostic
				result = diagnostic
				return result if result.is_a? Err
				g.diags.push result
				puts result
			when Token::Kind::RBrace then break
			else return Err.unexpected_token t, "'diagnostic' keyword"
			end
		end
		next_token
		return g
	end

	def diagnostic : Diagnostic | Err
		t = next_token
		return t if t.is_a? Err
		unless t.kind == Token::Kind::Word
			return Err.unexpected_token t, "name for diagnositc"
		end

		start_token = t

		name = t.data.as(String)
	
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
			when Token::Kind::RBrace then break
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

		return Diagnostic.new name, type, message
	end
end

p = Parser.new "src/generators/diagnostics.def"
case result = p.start
when Err then puts result
else
	p.groups.each do |g|
		puts g
	end
end
