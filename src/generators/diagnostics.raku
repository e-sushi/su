
my $time = DateTime.now(
    formatter=>{
        sprintf "%04d/%02d/%02d %02d:%02d:%02d", .year, .month, .day, .hour, .minute, .second given $^self
    });

my $file = 'diagnostics.def'.IO.slurp;

chdir '..';

my @groups = ();

my $line = 1;
my $col = 1;

parse-group;

sub skip-whitespace {
    loop {
        if $file ~~ /^\n/ {
            $line += 1;
            $col = 1;
            $file = $file.substr(1, *);
        }
        my $m = $file ~~ m/^\h+/;
        last unless $m;
        $col += $m.chars;
        $file = $file.substr($m.chars, *);
    }
}

sub expect-and-eat($re) {
    skip-whitespace;
    my $dinner = $file ~~ $re or die "expected {$re.raku} at $line:$col";
    my $out = $file.substr(0, $dinner.chars);
    $file = $file.substr($dinner.chars, *);
    $col += $out.chars;
    skip-whitespace;
    return $out;
}

sub parse-group {
    while $file {
        expect-and-eat /^group/;
        my $id = expect-and-eat /^\w+/;
        expect-and-eat /^'{'/;
        my $group = (group => [$id]);
        my @data := $group.value;

        until $file ~~ /^'}'/ or not $file {
            my $type = expect-and-eat /^[diagnostic || group]/;
            given $type {
                when $type eq 'group' {@data.push: parse-group}
                when $type eq 'diagnostic' {@data.push: parse-diag}
            }
        }
        expect-and-eat /^'}'/;
        @groups.push: $group;
    }
}

sub parse-diag {
    my $id = expect-and-eat /^\w+/;
    expect-and-eat /^'{'/;  
    my $diag = (diagnostic => [$id]);
    my @data := $diag.value;

    while 1 {
        last if $file ~~ /^'}'/;
        my $key = expect-and-eat /^[ type<|w> || message<|w> ]/;
        given $key {
            when 'type' {
                expect-and-eat /^':'/;
                my $val = expect-and-eat /^[ error<|w> || warning<|w> ]/;
                @data.push: (type => $val);
                expect-and-eat /^';'/;
            }
            when 'message' {
                expect-and-eat /^':'/;
                expect-and-eat /^'{'/;
                my @messages = ();
                until $file ~~ /^\s*'}'/ {
                    my $key = expect-and-eat /^\w+/;
                    expect-and-eat /^':'/;
                    my $val = expect-and-eat /^'"'.*?<!before \\>'"'/;
                    $val ~~ s/'"'(.*?)<!before \\>'"'/$0/;
                    @messages.push: ($key => $val);
                    expect-and-eat /^';'/;
                }
                @data.push: (messages => @messages);
                expect-and-eat /^'}'/;
            }
        }
    }
    expect-and-eat /^'}'/;
    $diag
}

my $count = 0;
my $header = 
    "/* {$time.Instant.to-posix[0].Int} [$time]\n" ~
    "    generated by diagnostics.raku from diagnostics.def\n" ~
    "*/\n";
my $funcs = 
    "namespace amu \{\n" ~
    "namespace diagnostic \{\n" ~
    "language lang;\n";
my $enum = "enum kind \{\n";
my $strs = "const global String strings[] = \{\n";

my $enum-prefix = "";

construct-diagnostics;

sub construct-diagnostics {
    for @groups -> $group {
        construct-group($group.value);
    }
}

sub construct-group($group) {
    $funcs ~= "namespace {$group[0]} \{\n"; 
    $enum-prefix ~= "{$group[0]}_";
    for $group[1..*] -> $child {
        when $child.key eq 'group' {construct-group($child.value)}
        when $child.key eq 'diagnostic' {construct-diag($child.value)}
    }
    $enum-prefix = $enum-prefix.chop: $group[0].chars+1;
    $funcs ~= "\} // namespace {$group[0]}\n\n";
}

sub construct-diag(@diag) {
    $enum ~= "\t$enum-prefix" ~ "{@diag[0]} = $count,\n";
    $strs ~= "\t\"$enum-prefix" ~ "{@diag[0]}\",\n";
    
    my $type;
    my @messages;

    for @diag[1..*] -> $child {
        when $child.key eq 'type' { $type = $child.value; }
        when $child.key eq 'messages' { @messages = $child.value; }
    }

    die "type not defined for {@diag[0]}" without $type;
    die "message not defined for {@diag[0]}" without @messages;

    my @dynparts = [];
    my @locales = [];
    for @messages -> $message {
        # split message where types are found 
        my @parts = $message.value.split(/\%\w+\%/, :v).grep(/.+/);
        my @locdynparts = [];
        my $argcount = 0;
        for @parts -> $part is rw {
            if $part.starts-with: '%' {
                given $part.substr(1,*-1) {
                    when 'String' {
                        @locdynparts.push: 'String';
                        $part = "arg$argcount";
                    }
                    when 'identifier' {
                        @locdynparts.push: 'String';
                        $part = "message::identifier(arg$argcount)";
                    }
                    when 'path' {
                        @locdynparts.push: 'String';
                        $part = "message::path(arg$argcount)";
                    }
                    when 'token' {
                        @locdynparts.push: 'Token*';
                        $part = "arg$argcount";
                    }
                }
            } else {
                $part = 'String("' ~ $part ~ '")';
            }
        }

        once @dynparts = @locdynparts;

        die "in '{@diag[0]}': locale '{$message.key}' uses a different amount of dynamic parts than the previous messages"
            if @dynparts !(==) @locdynparts;

        die "in '{@diag[0]}': unknown locale '{$message.key}'" 
            if not $message.key eq any('en', 'jp');
        
        @locales.push: ($message.key => @parts);
    }

    say @dynparts;

    my $sig = "";
    for @dynparts.kv -> $num, $part {
        $sig ~= ", $part arg{$num}";
    }

    my @message-commands = [];

    my $argcount = 0;
    for @locales -> $locale {

        my $command = "case {$locale.key}: \{\n";
        for $locale.value -> $part {
            # when $part eq '%%' {
            #     $command ~= "arg$argcount, ";
            # }
            $command ~= "\t\t\tmessage::push(out, $part);\n";
        }
        $command ~= "\t\t} break;";
        @message-commands.push: $command;
    }
    
    $funcs ~= 
    "FORCE_INLINE global void\n{@diag[0]}(MessageSender sender$sig) \{\n" ~
    "\tDiagnostic diag = \{0\};\n" ~
    "\tdiag.code = $count;\n" ~
    "\tdiag.sender = sender;\n" ~
    "\tdiag.severity = diagnostic::$type;\n" ~
    "\tarray::push(sender.source->diagnostics, diag);\n" ~
    "\tMessage out = message::init();\n" ~
    "\tout.kind = message::$type;\n" ~
    "\tswitch(lang) \{\n";
    for @message-commands -> $command {
        $funcs ~= "\t\t$command\n";
    }
    $funcs ~= 
    "\t}\n" ~
    "\tout = message::attach_sender(sender, out);\n" ~
    "\tmessenger::dispatch(out);\n" ~
    "}\n\n";
    $count += 1;
}



$funcs ~= "\} // namespace diagnostic\n\} // namespace amu";
$funcs = $header ~ $funcs;
($enum, $strs) >>~=>> "\};";
my $data = $header ~ $enum ~ "\n\n" ~ $strs;
$data ~= 
"\n\nenum language \{\n" ~
"\ten,\n" ~
"\tjp,\n" ~
"};\n\n" ~
"enum severity \{\n" ~
"\terror,\n" ~
"\twarning,\n" ~
"};\n";

my $funcs-out = open "../src/data/diagnostic-impl.generated", :w or die "unable to open impl file";
my $data-out = open "../src/data/diagnostics-data.generated", :w or die "unable to open data file";

$funcs-out.say($funcs);
$data-out.say($data);