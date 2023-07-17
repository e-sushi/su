# use Grammar::Tracer;

chdir "../src";

my $file = q:to/stop/;
group path {
    diagnostic hello {
        type: error;
        message: {
            en: "hi";
        }
    }
}
stop


# this seems to work well, but only when the input is correct, otherwise the parse will just fail and 
# there's no way (that I'm aware of) to catch errors and report them
grammar DiagnosticGrammar {
    rule  TOP          { [ <group> ]+ }
    rule  group        { 'group' <id> '{' [ <group-things> ]+ '}' || <error> }
    rule  group-things { <group> || <diag>}
    rule  diag         { 'diagnostic' <id> '{' [ <message> <type> || <type> <message> ] '}' }
    rule  diagkeys     { <message> || <type> }
    rule  message      { 'message' ':' '{' [ <locale> ]+ '}' }
    rule  locale       { <id> ':' <string> ';' }
    rule  type         { 'type' ':' ( 'error' || 'warning' ) ';' }
    regex string       { \"(.*?)<!before \\>\" }
    token id           { \w+ }
    rule  error        { .*?[ '{' | ';' ] }
}

class DiagnosticActions {
    has @.groups;

    method TOP($/) { make gather take $_.made for $<group>; }
    method group($/) { 
        make (group => ($<id>.made, [,] gather take $_.made for $<group-things>)); 
    }

    method group-things($/) {
        make $<group>.made if $<group>;
        make $<diag>.made if $<diag>;
    }

    method diag($/) {
        make (diagnostic => ($<id>.made, $<type>.made, $<message>.made));
    }

    # NOTE(sushi) in 'message' we are using Raku's reduce operator [*], which takes a sequence on the right
    #             and applies whatever is inside of '[]' between each element
    #             what that does here is generate a List from the locales we gather, because 
    #             '1, 2, 3' is a list: (1,2,3)
    method message($/) { 
        make [,] gather take $_.made for $<locale>;
    }
    method locale($/) { make ($<id>.made => $<string>.made); } # make a pair 'id => string'
    method type($/)   { make (type => $/.caps[0].value); } # make a pair 'type => captured val'
    method string($/) { make $/.caps[0].value; } # extract the capture inside of the quotes
    method id($/)     { make $/.Str; } 
    method error($/) {

    }
}

my @parse = DiagnosticGrammar.subparse($file, actions => DiagnosticActions).made; 
say @parse;
my $funcs-out = open "data/diagnostic-funcs.generated", :w or die "unable to open output file";
my $enum-out  = open "data/diagnostic-enum.generated", :w or die "unable to open enum file";
my $str-out   = open "data/diagnostic-strings.generated", :w or die "unable to open strings file";

my $count = 0;
my $funcs = "namespace amu \{\nnamespace diagnostic \{\n";
my $enum  = "enum kind \{\n";
my $strs  = "const global String strings[] = \{\n";

my $enum-prefix = "";

write-group($_.value) for @parse;

sub write-group($group) {
    $funcs ~= "namespace {$group[0]} \{\n\n";
    $enum-prefix ~= "{$group[0]}_";
    my @children = $group[1];
    for @children -> $child {
        write-diagnostic($child.value) when $child.key eq 'diagnostic';
    }
    $enum-prefix = $enum-prefix.chop: $group[0].chars;
    $funcs ~= "\} // namespace {$group[0]}\n\n";
}

sub write-diagnostic($diag) {
    #extract any arguments the diagnostic wants 

    $enum ~= "\t$enum-prefix" ~ "{$diag[0]} = $count,\n";
    $strs ~= "\t\"$enum-prefix" ~ "{$diag[0]}\",\n";
    $funcs ~=
    "FORCE_INLINE global void\n{$diag[0]}(MessageSender sender) \{\n" ~
    "\tDiagnostic diag = \{0\};\n" ~
    "\tdiag.code = $count;\n" ~
    "\tdiag.sender = sender;\n" ~
    "\tarray::push(sender.source->diagnostics, diag);\n" ~
    "}\n\n";

    $count += 1;
}

$enum ~= "};";
$strs ~= "};";
$funcs ~= "} // namespace diagnostic\n} // namespace amu";


$funcs-out.say($funcs);
$enum-out.say($enum);
$str-out.say($strs);

# $out.say(
# "\} // namespace diagnostic\n" ~
# "\} // namespace amu"
# );

