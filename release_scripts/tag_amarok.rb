#!/usr/bin/env ruby
#
# This script creates a new tag in SVN for Amarok. Upon startup, it asks you for name
# of the tag to create (e.g. "1.3-beta3"). Please note that the script creates the tag
# immediately on the live SVN repository.
#
# (c) 2005 Mark Kretschmann <markey@web.de>
# License: GNU General Public License V2


useStableBranch = false
unless $*.empty?()
    case $*[0]
        when "--branch"
            useStableBranch = true
        else
            puts("Unknown option #{$1}. Known options: --branch.\n")
    end
end

# Ask user for tag name
tagname  = `kdialog --inputbox "Enter tag name (e.g. "1.3-beta3"): "`.chomp()
user = `kdialog --inputbox "Your SVN user:"`.chomp()
protocol = `kdialog --radiolist "Do you use https or svn+ssh?" https https 0 "svn+ssh" "svn+ssh" 1`.chomp()

# Show safety check dialog
`kdialog --warningcontinuecancel "Really create the tag '#{tagname}' NOW in the svn repository?"`
if $?.exitstatus() == 2
    print "Aborted.\n"
    exit()
end

# Create destination folder
target = "#{protocol}://#{user}@svn.kde.org/home/kde/tags/amarok/#{tagname}/"
`svn mkdir -m "Create tag #{tagname} root directory" #{target}`
`svn mkdir -m "Create tag #{tagname} multimedia directory" #{target}/multimedia`
`svn mkdir -m "Create tag #{tagname} doc directory" #{target}/multimedia/doc`

if useStableBranch
    source = "#{protocol}://#{user}@svn.kde.org/home/kde/branches/stable/extragear/multimedia/amarok"
    docs   = "#{protocol}://#{user}@svn.kde.org/home/kde/branches/stable/extragear/multimedia/doc/amarok"
else
    source = "#{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia/amarok"
    docs   = "#{protocol}://#{user}@svn.kde.org/home/kde/trunk/extragear/multimedia/doc/amarok"
end


# Copy the files in the repository
`svn cp -m "Tag Amarok #{tagname}." #{source} #{target}/multimedia`
`svn cp -m "Tag Amarok #{tagname} - docs." #{docs} #{target}/multimedia/doc`


print "Tag created.\n"
