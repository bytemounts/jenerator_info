using generator_web.Models;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using System.Security.Claims;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authentication.Cookies;
using System.Reflection.Metadata;
using System;
using System.Xml.Linq;

namespace generator_web.Controllers
{
    public class LoginController : Controller
    {
        private readonly AppDbContext _context;
        private readonly IPasswordHasher<User> _hasher;

        public LoginController(AppDbContext context, IPasswordHasher<User> hasher)
        {
            _context = context;
            _hasher = hasher;
        }

        // ================= REGISTER =================

        // GET: /Login/Register
        public IActionResult Register()
        {
            return View();
        }

        // POST: /Login/Register
        [HttpPost]
        public async Task<IActionResult> Register(string username, string email, string password)
        {
            if (string.IsNullOrWhiteSpace(username) || string.IsNullOrWhiteSpace(email) || string.IsNullOrWhiteSpace(password))
            {
                ViewBag.Error = "Tous les champs sont requis ❌";
                return View();
            }

            // Vérifier si utilisateur existe déjà
            var existingUser = await _context.Users.FirstOrDefaultAsync(u => u.Username == username || u.email == email);
            if (existingUser != null)
            {
                ViewBag.Error = "Un utilisateur avec ce nom ou email existe déjà ❌";
                return View();
            }

            var user = new User
            {
                Username = username,
                email = email
            };

            // Hachage du mot de passe
            user.PasswordHash = _hasher.HashPassword(user, password);

            _context.Users.Add(user);
            await _context.SaveChangesAsync();

            TempData["Message"] = "Inscription réussie ✅ Connecte-toi maintenant.";
            return RedirectToAction("Login");
        }

        // ================= LOGIN =================

        // GET: /Login/Login
        public IActionResult Login()
        {
            return View();
        }

        // POST: /Login/Login
        [HttpPost]
        public async Task<IActionResult> Login(string username, string password)
        {
            if (string.IsNullOrWhiteSpace(username) || string.IsNullOrWhiteSpace(password))
            {
                ViewBag.Error = "Tous les champs sont requis ❌";
                return View();
            }

            var user = await _context.Users.FirstOrDefaultAsync(u => u.Username == username);
            if (user == null)
            {
                ViewBag.Error = "Nom d’utilisateur ou mot de passe incorrect ❌";
                return View();
            }

            var result = _hasher.VerifyHashedPassword(user, user.PasswordHash, password);
            if (result == PasswordVerificationResult.Success ||
                result == PasswordVerificationResult.SuccessRehashNeeded)
            {
                // 🔑 Create les claims
                var claims = new List<Claim>
                {
                    new Claim(ClaimTypes.Name, user.Username),
                    new Claim(ClaimTypes.Email, user.email),
                    //new Claim("UserId", user.UserId.ToString()),
                    new Claim(ClaimTypes.NameIdentifier, user.UserId.ToString())
                };

                var claimsIdentity = new ClaimsIdentity(claims, CookieAuthenticationDefaults.AuthenticationScheme);

                var authProperties = new AuthenticationProperties
                {
                    IsPersistent = true,
                    ExpiresUtc = DateTimeOffset.UtcNow.AddHours(1)
                };

                await HttpContext.SignInAsync(
                    CookieAuthenticationDefaults.AuthenticationScheme,
                    new ClaimsPrincipal(claimsIdentity),
                    authProperties
                );

                // 🔍 DÉBOGAGE - Vérifier si le cookie est créé
                Console.WriteLine("=== APRÈS SIGNIN ===");
                Console.WriteLine($"IsAuthenticated: {HttpContext.User.Identity?.IsAuthenticated}");
                Console.WriteLine($"User Name: {HttpContext.User.Identity?.Name}");
                Console.WriteLine($"Claims count: {HttpContext.User.Claims.Count()}");

                foreach (var claim in HttpContext.User.Claims)
                {
                    Console.WriteLine($"  Claim: {claim.Type} = {claim.Value}");
                }

                // Vérifier les cookies
                var cookies = HttpContext.Response.Headers["Set-Cookie"];
                Console.WriteLine($"Set-Cookie headers: {cookies}");
                Console.WriteLine("===================");

                return RedirectToAction("Index", "Home"); // redirection après login


            }

            ViewBag.Error = "Nom d’utilisateur ou mot de passe incorrect ❌";
            return View();
        }

        // ================= LOGOUT =================

        public async Task<IActionResult> Logout()
        {
            await HttpContext.SignOutAsync(CookieAuthenticationDefaults.AuthenticationScheme);
            return RedirectToAction("Login");
        }
    }
}

