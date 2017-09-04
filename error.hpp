/* error.hpp -- handling errors/warnings etc.
 * by pts@fazekas.hu at Fri Mar  1 11:32:36 CET 2002
 */

#ifdef __GNUC__
#ifndef __clang__
#pragma interface
#endif
#endif

#ifndef ERROR_HPP
#define ERROR_HPP 1

#include "config2.h"
#include "gensi.hpp"

/** This may be usafe if an error happens inside an error. Usage:
 * Error::sev(Error::WARNING) << "Zero-length image." << (Error*)0;
 */
class Error {
 public:
  static char const* banner0;
  static char const* argv0;
  static char const* tmpargv0;
  static char const* long_argv0;
  /** Error types. */
  BEGIN_STATIC_ENUM(int,level_t)
    ASSERT=4, FATAL=3,
    EERROR=2, /* ERROR conflicts with the Win32 API :-( */
    ERROR_CONT=-2, /* error, but continue running program */
    WARNING=-3,
    WARNING_DEFER=-4, /* warning, but defer (and later possibly omit) displaying it */
    NOTICE=-5,
    NOTICE_DEFER=-6, INFO=-111, DEBUG=-222
  END_STATIC_ENUM()
  /** Standard error stream (stderr) of the current process. */
  static GenBuffer::Writable * serr;
  /** Returns address to statically allocated buffer. */
  static char const*level2str(level_t level);
  /** This is the (carefully hidden :-)) method of raising errors (i.e
   * displaying error messages and terminating the program).
   * Example usage: 
   *   Error::sev(Error::WARNING_DEFER) << "check_rule: /BMP requires "
   *                                       "/Predictor " << 1 << (Error*)0;
   * The error is interpreted according to the current policy. The policy
   * divides errors into three categories:
   *
   * -- secret: nothing happens (even the error message is _not_ printed)
   * -- recorded: nothing happens (even the error message is _not_ printed),
   *    but the message is remembered for further processing.
   * -- printed: the error message is printed, and program execution continues
   *    Before printing this message, all recorded errors on the policy stack
   *    are also printed (deepest first).
   * -- killer: like `printed', but runs cleanup handlers and terminates the
   *    program immediately.
   *
   * @param level is one of the constants mentioned above (FATAL, EERROR,
   *   WARNING, NOTICE etc). Can be positive, zero or negative. The larger
   *   the `level', the more severe the error is. The default policy is:
   *   level>=0 errors are killer, -99<=level<=-1 errors are printed,
   *   -199<=level<=-100 are recorded and level<=-200 errors are secret.
   */
  static GenBuffer::Writable& sev(level_t level);
  
 public:
  /** The field order is important in this struct, because of the initializers. */
  struct Policy {
    /** All recorded messages. Default: NULL. This means empty. */
    SimBuffer::B *record;
    /** The highest level of Secret category encountered so far. Default: -9999 */
    level_t topSecret;
    /** The highest level of Recorded category encountered so far. Default: -9999 */
    level_t topRecorded;
    /** Lower bound of these categories in this policy. */    
    level_t killer, printed, recorded;
    /** Error stream to print printed and killer messages. */
    GenBuffer::Writable *err;
    /** NULL for top policy */
    Policy *prev, *next;
    /** Level of the current error being raised. */
    level_t curlev;
  };
 protected:
  /** Boundaries of the policy stack. */
  static Policy *policy_top, *policy_bottom;  
 public:
  friend GenBuffer::Writable& operator <<(GenBuffer::Writable&,Error*);
  /** Creates a new policy and makes it active by pushing it onto the top of
   * the policy stack.
   */
  static void pushPolicy(level_t killer_, level_t printed_, level_t recorded_, GenBuffer::Writable *err=(GenBuffer::Writable*)NULLP);
  /** @return the messages already recorded, and clears the `record' entry
   * of the current policy. The caller is responsible for deleting the
   * pointer returned. May return NULLP for empty records. Typical example:
   *   delete Error::getRecorded();
   */
  static SimBuffer::B *getRecorded();
  /** Prints all recorded error messages, and removes the topmost element of
   * the policy stack. Typical example:
   *   delete Error::getRecorded();  popPolicy();
   */
  static void popPolicy();

  /** @example Error::setTopPrinted(Error::ERROR_CONT); suppress anything
   * below error, such as warnings.
   */
  static void setTopPrinted(level_t printed_);
  
  static level_t getTopPrinted();
  
  /** The Cleanup mechanism is similar to atexit(3) and on_exit(3). This is
   * just a simple struct with no destructors, virtual methods or inheritance.
   *
   * Additional data (char[] buffer) can be allocated and stored right
   * after the object (this+1), it can be retrieved with getBuf(), and it will
   * be deleted when the object is deleted by runCleanups().
   */
  struct Cleanup {
    /** Must _not_ cause any Errors.
     * @return an exit code. If larger than the current one, replaces it
     */
    typedef int (*handler_t)(Cleanup*);
    /** Owned externally. */
    handler_t handler;
    /** size of extra data allocated at getBuf(). */
    slen_t bufSize;
    /** Arbitrary data, owned by `handler': handler' must delete `data'
     * when called.
     */
    void *data;
    /** NULLP: no next, end of chain */
    Cleanup *next;
    inline char *getBuf()   { return (char*)(this+1); }
    inline slen_t getSize() { return bufSize; }
  };
  /** Creates and returns a new Cleanup, and registers it in front of the
   * existing ones. Copies `handler' and `data' to it. Also allocates `bufSize' bytes
   * of char[] buffer (at result->getBuf()) owned by the cleanup, but doesn't
   * initialize the buffer.
   *
   * Doesn't take ownership of `handler', takes ownership of `data'.
   * `handler' must delete `data' or pass on ownership when called,
   * typically by runCleanups().
   */
  static Cleanup* newCleanup(Cleanup::handler_t handler, void *data, slen_t bufSize);
  /** Creates and returns a new Cleanup, and registers it in front of the
   * existing ones. Copies `handler' and `data' to it. Also allocates
   * strlen(bufCstr)+1 bytes of char[] buffer (at result->getBuf()) owned by
   * the cleanup, and initializes it from bufCstr.
   *
   * Doesn't take ownership of `handler', takes ownership of `data'.
   * `handler' must delete `data' or pass on ownership when called,
   * typically by runCleanups().
   */
  static Cleanup* newCleanup(Cleanup::handler_t handler, void *data, char const*bufCstr);
  /** Reverts to the default logging policy,
   * executes the cleanup handlers (in reverse-registration order),
   * unregisters and deletes all the cleanups, and returns the
   * new exit code, which is at least exitCode.
   */
  static int runCleanups(int exitCode);
  /* Exits from the process with the specified exit code. */
  static void cexit(int exitCode);
 protected:
  static Cleanup *first_cleanup;
};

GenBuffer::Writable& operator <<(GenBuffer::Writable&,Error*);
 
#endif
