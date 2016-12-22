/*---------------------------------------------------------------------------*\
                          ____  _ _ __ _ __  ___ _ _
                         |_ / || | '_ \ '_ \/ -_) '_|
                         /__|\_, | .__/ .__/\___|_|
                             |__/|_|  |_|
\*---------------------------------------------------------------------------*/

#ifndef ZYPPER_H
#define ZYPPER_H

#include <iostream>
#include <string>
#include <vector>

#include <zypp/base/Exception.h>
#include <zypp/base/NonCopyable.h>
#include <zypp/base/PtrTypes.h>
#include <zypp/base/Flags.h>
#include <zypp/TriBool.h>

#include <zypp/RepoInfo.h>
#include <zypp/RepoManager.h> // for RepoManagerOptions
#include <zypp/SrcPackage.h>
#include <zypp/TmpPath.h>

#include "Config.h"
#include "Command.h"
#include "utils/getopt.h"
#include "output/Out.h"

// As a matter of fact namespaces std, boost and zypp have overlapping
// symbols (e.g. shared_ptr). We default to the ones used in namespace zypp.
// Symbols from other namespaces should be used explicitly (std::set, boost::format)
// and not by using the whole namespace.
using namespace zypp;

// Convenience
using std::cout;
using std::cerr;
using std::endl;

struct Options;

/** directory for storing manually installed (zypper install foo.rpm) RPM files
 */
#define ZYPPER_RPM_CACHE_DIR "/var/cache/zypper/RPMS"

/**
 * Structure for holding global options.
 *
 * \deprecated To be replaced by Config
 */
struct GlobalOptions
{
  GlobalOptions()
  :
  verbosity(0),
  disable_system_sources(false),
  disable_system_resolvables(false),
  is_rug_compatible(false),
  non_interactive(false),
  reboot_req_non_interactive(false),
  no_gpg_checks(false),
  gpg_auto_import_keys(false),
  machine_readable(false),
  no_refresh(false),
  no_cd(false),
  no_remote(false),
  root_dir("/"),
  no_abbrev(false),
  terse(false),
  changedRoot(false)
  {}

//  std::list<zypp::Url> additional_sources;

  /**
   * Level of the amount of output.
   *
   * <ul>
   * <li>-1 quiet</li>
   * <li> 0 normal (default)</li>
   * <li> 1 verbose</li>
   * <li> 2 debug</li>
   * </ul>
   */
  int verbosity;
  bool disable_system_sources;
  bool disable_system_resolvables;
  bool is_rug_compatible;
  bool non_interactive;
  bool reboot_req_non_interactive;
  bool no_gpg_checks;
  bool gpg_auto_import_keys;
  bool machine_readable;
  /** Whether to disable autorefresh. */
  bool no_refresh;
  /** Whether to ignore cd/dvd repos) */
  bool no_cd;
  /** Whether to ignore remote (http, ...) repos */
  bool no_remote;
  std::string root_dir;
  zypp::RepoManagerOptions rm_options;
  bool no_abbrev;
  bool terse;
  bool changedRoot;
};

/**
 * \bug The RepoInfo lists kept herein may lack housekeeping data added by the
 * zypp::RepoManager. Consider using your own RepoInfos only for those not
 * maintained by zypp::RepoManager. (bnc #544432)
*/
struct RuntimeData
{
  RuntimeData()
    : patches_count(0), security_patches_count(0)
    , show_media_progress_hack(false)
    , force_resolution(zypp::indeterminate)
    , solve_before_commit(true)
    , commit_pkgs_total(0)
    , commit_pkg_current(0)
    , seen_verify_hint(false)
    , action_rpm_download(false)
    , waiting_for_input(false)
  {}

  std::list<zypp::RepoInfo> repos;
  std::list<zypp::RepoInfo> additional_repos;
  int patches_count;
  int security_patches_count;
  /**
   * Used by requestMedia callback
   * \todo but now it uses label, remove this variable?
   */
  zypp::RepoInfo current_repo;

  std::set<zypp::SrcPackage::constPtr> srcpkgs_to_install;

  // hack to enable media progress reporting in the commit phase in normal
  // output level
  bool show_media_progress_hack;

  // Indicates an ongoing raw meta-data refresh.
  // If not empty call zypper.out().progress(
  //  "raw-refresh", raw_refresh_progress_label) in the media download
  // progress callback.
  //! \todo better way to do this would be to propagate the download progress
  // all the way up from the media back-end through fetcher and downloader
  // into the RepoManager::refreshMetadata(), so that we get a combined percentage
  std::string raw_refresh_progress_label;

  /** Used to override the command line option */
  zypp::TriBool force_resolution;

  /**
   * Set to <tt>false</tt> to avoid calling of the solver
   * in \ref solve_and_commit(). Needed after Resolver::doUpdate()
   */
  bool solve_before_commit;

  unsigned int commit_pkgs_total;
  unsigned int commit_pkg_current;

  bool seen_verify_hint;
  bool action_rpm_download;

  //! \todo move this to a separate Status struct
  bool waiting_for_input;

  //! Temporary directory for any use. Used e.g. as packagesPath of TMP_RPM_REPO_ALIAS repository.
  zypp::filesystem::TmpDir tmpdir;
};

typedef zypp::shared_ptr<zypp::RepoManager> RepoManager_Ptr;

class Zypper : private zypp::base::NonCopyable
{
public:
  typedef zypp::RW_pointer<Zypper,zypp::rw_pointer::Scoped<Zypper> > Ptr;
  typedef std::vector<std::string>  ArgList;

  static Ptr & instance();

  int main(int argc, char ** argv);

  // setters & getters
  Out & out();
  void setOutputWriter(Out * out) { _out_ptr = out; }
  Config & config() { return _config; }
  const GlobalOptions & globalOpts() const { return _gopts; }
  GlobalOptions & globalOptsNoConst() { return _gopts; }
  const parsed_opts & cOpts() const { return _copts; }
  const ZypperCommand & command() const { return _command; }
  const std::string & commandHelp() const { return _command_help; }
  const ArgList & arguments() const { return _arguments; }
  RuntimeData & runtimeData() { return _rdata; }

  zypp::RepoManager & repoManager()
  { if (!_rm) _rm.reset(new zypp::RepoManager(_gopts.rm_options)); return *_rm; }

  void initRepoManager()
  { _rm.reset(new zypp::RepoManager(_gopts.rm_options)); }

  int exitCode() const				{ return _exitCode; }
  void setExitCode( int exit )			{ _exitCode = exit; }

  int refreshCode() const			{ return _refreshCode; }
  void setRefreshCode( int exit )		{ _refreshCode = exit; }

  bool runningShell() const			{ return _running_shell; }
  bool runningHelp() const			{ return _running_help; }
  bool exitRequested() const			{ return _exit_requested; }
  void requestExit( bool do_exit = true )	{ _exit_requested = do_exit; }

  int argc() { return _running_shell ? _sh_argc : _argc; }
  char ** argv() { return _running_shell ? _sh_argv : _argv; }

  void cleanup();

public:
   /** Flags for tuning \ref defaultLoadSystem. */
  enum _LoadSystemFlags
  {
    NO_TARGET		= (1 << 0),		//< don't load target to pool
    NO_REPOS		= (1 << 1),		//< don't load repos to pool
    NO_POOL		= NO_TARGET | NO_REPOS	//< no pool at all
  };
  ZYPP_DECLARE_FLAGS( LoadSystemFlags, _LoadSystemFlags );

  /** Prepare repos and pool according to \a flags_r.
   * Defaults to load target and repos and in this case also adjusts
   * the PPP status by doing an initial solver run.
   */
  int defaultLoadSystem( LoadSystemFlags flags_r = LoadSystemFlags() );

public:
  /** Convenience to return properly casted _commandOptions. */
  template<class _Opt>
  shared_ptr<_Opt> commandOptionsAs() const
  { return dynamic_pointer_cast<_Opt>( _commandOptions ); }

  /** Convenience to return _commandOptions or default constructed Options. */
  template<class Opt_>
  shared_ptr<Opt_> commandOptionsOrDefaultAs() const
  {
    shared_ptr<Opt_> myopt = commandOptionsAs<Opt_>();
    if ( ! myopt )
      myopt.reset( new Opt_() );
    return myopt;
  }

  /** Convenience to return command options for \c _Op, either casted from _commandOptions or newly created. */
  template<class _Opt>
  shared_ptr<_Opt> assertCommandOptions()
  {
    shared_ptr<_Opt> myopt( commandOptionsAs<_Opt>() );
    if ( ! myopt )
    {
      myopt.reset( new _Opt() );
      _commandOptions = myopt;
    }
    return myopt;
  }

public:
  ~Zypper();
private:
  Zypper();

  void processGlobalOptions();
  void processCommandOptions();
  void commandShell();
  void shellCleanup();
  void safeDoCommand();
  void doCommand();

  void setCommand(const ZypperCommand & command) { _command = command; }
  void setRunningShell(bool value = true) { _running_shell = value; }
  void setRunningHelp(bool value = true) { _running_help = value; }

private:

  int     _argc;
  char ** _argv;

  Out * _out_ptr;
  Config _config;
  GlobalOptions _gopts;
  parsed_opts   _copts;
  ZypperCommand _command;
  ArgList _arguments;
  std::string _command_help;

  int   _exitCode;
  int   _refreshCode;	// do_init_repos hack!
  bool  _running_shell;
  bool  _running_help;
  bool  _exit_requested;

  RuntimeData _rdata;

  RepoManager_Ptr   _rm;

  int _sh_argc;
  char **_sh_argv;

  /** Command specific options (see also _copts). */
  shared_ptr<Options>  _commandOptions;
};

/** \relates Zypper::LoadSystemFlags */
ZYPP_DECLARE_OPERATORS_FOR_FLAGS( Zypper::LoadSystemFlags );

void print_main_help(const Zypper & zypper);
void print_unknown_command_hint(Zypper & zypper);
void print_command_help_hint(Zypper & zypper);

///////////////////////////////////////////////////////////////////
/// \brief Base class for command specific option classes.
///////////////////////////////////////////////////////////////////
struct Options
{
  //Options() : _command( "" ) {}      // FIXME: DefaultCtor is actually undesired!
  Options( const ZypperCommand & command_r ) : _command( command_r ) {}
  virtual ~Options() {}

  /** The command. */
  const ZypperCommand & command() const
  { return _command; }

  /** The command name (optionally suffixed). */
  std::string commandName( const std::string & suffix_r = std::string() ) const
  { std::string ret( _command.asString() ); if ( ! suffix_r.empty() ) ret += suffix_r; return ret; }

  /** The command help text written to a stream. */
  virtual std::ostream & showHelpOn( std::ostream & out ) const        // FIXME: become pure virtual
  {
    out
      << _command << " ...?\n"
      << "This is just a placeholder for a commands help.\n"
      << "Please file a bug report if this text is displayed.\n"
      ;
    return out;
  }

  /** The command help as string. */
  std::string helpString() const
  { std::ostringstream str; showHelpOn( str ); return str.str(); }

  /** Show user help on command. */
  void showUserHelp( Zypper & zypper_r ) const
  { zypper_r.out().info( helpString(), Out::QUIET ); } // always visible

private:
  ZypperCommand _command;      //< my command
};

class ExitRequestException : public zypp::Exception
{
public:
  ExitRequestException(const std::string & msg = "") : zypp::Exception(msg) {}
};

#endif /*ZYPPER_H*/

// Local Variables:
// c-basic-offset: 2
// End:
