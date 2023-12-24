

start_time = Time.monotonic 

require "colorize"

# platform check at compile time
{% if flag? :win32 %}
{% 	raise("building on win32 is not setup yet (I don't have a windows environment to test in rn)") %}
{% elsif !flag?(:linux) %}
{%	raise("building is currently only supported on linux and win32") %}
{% end %}

struct Time
	def my_format
		self.to_s "%a, %b %-d %Y %H:%M:%S %^Z"
	end
end

struct Time::Span
	def pretty 
		String.build do |str|
			if minutes != 0
				str << "#{minutes}m "
			end
			if seconds != 0
				str << "#{seconds}s "
			end
			if milliseconds != 0
				str << "#{milliseconds}ms "
			end
		end
	end
end

class Build

# configuration properties
property \
        platform = "",
         verbose = false,
       buildmode = "debug",
       profiling = "off",
  build_analysis = false,
         use_pch = false,
     gen_compcmd = false,
    preprocessor = "unknown",
        compiler = "unknown",
          linker = "unknown",
		max_jobs = System.cpu_count.as(Int64),
       build_dir = "build",
   hide_greeting = false

# other stuff
property \
	known_platforms = {"linux"},
known_preprocessors = {"cpp"},
	known_compilers = {"clang++"},
	  known_linkers = {"clang++"}

def vprint(x)
	if verbose
		puts x
	end
end

def fatal(x)
	puts <<-END 
	#{caller[1]}:
	#{"fatal".colorize.red}: #{x}
	END
	p self
	exit 1
end

def warn(x)
	print "warning".colorize.yellow, ": ", x, "\n"
end

# use the preprocessor to find all includes of a file
# then save a cache of them to the build folder
def cache_dependencies(filename, outfilename, defines, includes)
	case preprocessor
	when "cpp"
		defines = defines.join " "
		includes = includes.join " "

		dependencies = `cpp #{filename} #{defines} #{includes} -MM`
			.split(" ", remove_empty: true)
			.skip(2)
			.each
			.map { |x| x.strip }
			.select { |x| x != "\\" }
			.join "\n"
		File.write(outfilename, dependencies)
	else
		fatal "unimplemented preprocessor '#{preprocessor}' for 'cache_dependencies'"
	end
end

def clean_and_quit
	puts "rm -rfd #{build_dir}/*"
	puts `rm -rfd #{build_dir}/*`
	exit 0
end

# set default programs depending on the platform
def set_defaults_from_platform
{% if flag? :linux %}
	self.preprocessor = "cpp"
	self.compiler = "clang++"
	self.linker = "clang++"
	self.platform = "linux"
{% else %}
{%   raise "'defaults_from_platform' needs to be implemented for this platform " %}
{% end %}
end

def process_argv
	args = ARGV.each
	loop do
		case args.next
		when args.stop then break
		when "clean"  then clean_and_quit
		when "-v"     then verbose = true
		when "-time"  then time = true
		when "-r"     then buildmode = "release"
		when "-p"     then profiling = "on"
		when "-pw"    then profiling = "on and wait"
		when "-ba"    then build_analysis = true
		when "-pch"   then use_pch = true
		when "-cc"    then gen_compcmd = true
		when "-platform"
			plat = args.next
			if plat.is_a? Iterator::Stop
				fatal "expected a platform after '-platform'"
			end
			unless known_platforms.any? plat
				fatal <<-END
				unknown platform #{plat}. Known platformers are: #{known_platforms}
				New platforms can be implemented in tools/src/build.cr.
				END
			end
			self.platform = plat
		when "-preprocessor"
			p = args.next
			if p.is_a? Iterator::Stop
				fatal "expected a preprocessor after '-preprocessor'"
			end
			unless known_preprocessors.any? p
				fatal <<-END
				unknown preprocessor #{p}. Known preprocessors are: #{known_preprocessors}
				New preprocessors can be implemented in tools/src/build.cr.
				END
			end
			self.preprocessor = p
		when "-compiler"
			c = args.next
			if c.is_a? Iterator::Stop
				fatal "expected a compiler after '-compiler'"
			end
			unless known_compilers.any? c
				fatal <<-END 
				unknown compiler '#{c}'. Known compilers are: #{known_compilers}
				New compilers can be implemented in tools/src/build.cr.\n
				END
			end
			self.compiler = c
		when "-linker"
			l = args.next
			if l.is_a? Iterator::Stop
				fatal "expected a linker after '-linker'"
			end
			unless known_linkers.any? l
				fatal <<-END
				unknown linker '#{l}'. Known linkers are: #{known_linkers}
				New linkers can be implemented in tools/src/build.cr.\n
				END
			end
			linker = l
		when "-jobs"
			ns = args.next
			if ns.is_a? Iterator::Stop
				fatal "expected some number of jobs after '-jobs'"
			end
			if n = ns.to_i?
				unless n < System.cpu_count
					warn "the given number of jobs (#{n}) is greater than the number of logical cores on this system (#{System.cpu_count})"
				end
				self.max_jobs = n
			else
				fatal "expected a number after '-jobs', not "
			end
		end
	end
end

# any function after this point should only be called after args have been collected

def output_path
	Path[build_dir] / buildmode
end

def output_exe
	output_path / "amu"
end

# display a nice message
def greeting
	puts <<-END
	* --- #{Time.local.my_format} --- *
	#{"amu".colorize.green} [ #{compiler.colorize.blue}/#{linker.colorize.blue}/#{buildmode.colorize.blue} ]
	#{
		if File.exists? output_exe
			"last build: #{File.info(output_exe).modification_time.to_local.my_format}"
		end
	}
	END
end

@defines = [] of String

# build an array of defines
def defines
	if @defines.empty?
		@defines = case buildmode
		when "release" then %w()
		when "debug"   then %w(-DAMU_DEBUG)
		else fatal "unhandled buildmode"
		end |
		case platform
		when "linux" then %w(-DAMU_LINUX=1)
		else fatal "unhandled platform"
		end | 
		case profiling
		when "on" then %w(-DAMU_PROFILING=1)
		when "on and wait" then %w(-DAMU_PROFILING=1 -DAMU_PROFILING_WAIT=1)
		when "off" then %w()
		else fatal "unhandled profiling mode"
		end
	end
	@defines
end

@includes = [] of String

def includes
	if @includes.empty?
		@includes = case compiler
		when "clang++"
			%w(
				-Isrc
			)
		else fatal "unhandled compiler #{compiler}"
		end
	end
	@includes
end

# TODO(sushi) add libs generator when needed

def pch
	case compiler
	when "clang++"
		fatal "TODO(sushi) automatic pch generation and recompilation, ideally incremental like obj files"
	else 
		fatal "precompiled headers are not implemented for compiler '#{compiler}"
	end
end

@compiler_flags = [] of String

def compiler_flags
	if @compiler_flags.empty?
		@compiler_flags = case compiler
		when "clang++"
			%w(
			-std=c++20
			-finline-functions
			-pipe
			-msse3
			-fno-caret-diagnostics
			-fdiagnostics-color=always
			) | 
		   case buildmode
			when "release" then %w(-O2)
			when "debug" 
				%w(
				-fdebug-macro
				-ggdb3
				-O0
				)
			else fatal "unhandled buildmode"
			end
		else fatal "compiler flags not setup for '#{compiler}'"
		end
	end
	@compiler_flags
end

@source_files = [] of Path

def source_files
	if @source_files.empty?
		@source_files =
			Dir.glob("src/**/*.cpp").map {|x| Path[x].normalize }
	end
	@source_files
end

struct Command
	property \
	command = "",
	args = [] of String,
	from = "",
	to = ""
end	

@commands = [] of Command

def generate_compiler_commands
	case @compiler
	when "clang++"
		source_files.each do |sf|
			c = Command.new
			c.command = "clang++"
			c.from = sf.to_s
			c.to = (output_path / "#{sf.stem}.o").to_s
			c.args = ["-c", c.from, "-o", c.to, *@includes, *@defines, *@compiler_flags]
			@commands.push c
		end
	else fatal "unhandled compiler '#{@compiler}'"
	end
end

def execute_compiler_commands
	# commands are send to fibers via this channel
	# the end is signaled by nil
	command_channel = Channel(Command | Nil).new(@commands.size)
	# channel which fibers use to notify the main fiber that they
	# are done
	done_signal = Channel(Nil).new
	
	# sentinel value returned to indicate if we should try
	# linking or not
	success = false

	# spawn as many fibers as we are allowed jobs
	max_jobs.times do
		spawn do
			# pipes for this fiber to get process output from
			stdout, stderr = IO::Memory.new, IO::Memory.new
			loop do
				c = command_channel.receive
				break if c.is_a? Nil
				vprint "#{c.command} #{c.args.join " "}"
				start_time = Time.monotonic
				# start the process and then wait for it to finish
				proc = Process.new(c.command, c.args, output: stdout, error: stderr)
				result = proc.wait
				elapsed_time = Time.monotonic - start_time
				if result.success?
					print "#{c.from.colorize.cyan} -> #{c.to.colorize.green} #{elapsed_time.pretty}\n"
					# if we find a warning anywhere in the stderr, print it
					# TODO(sushi) setup doing this for various compilers
					if (t = stderr.to_s).includes? "warning"
						puts t
					end
				else
					success = false
					print <<-END
					#{c.from.colorize.yellow} #{"failed".colorize.red}:
					#{stderr}
					END
				end
				# signal the main fiber that we've finished a job
				done_signal.send(nil)
			end
		end
	end

	# queue up each command
	@commands.each do |c|
		command_channel.send c
	end
	command_channel.send nil

	# we expect one done signal for each command 
	# so we wait on the done signal that many times
	@commands.size.times { done_signal.receive }

	return success
end

def start
	set_defaults_from_platform
	process_argv
	greeting
	defines
	compiler_flags
	source_files
	generate_compiler_commands
	execute_compiler_commands
end

end

Build.new.start
