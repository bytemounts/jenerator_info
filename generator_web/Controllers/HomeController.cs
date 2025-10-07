using System.Diagnostics;
using generator_web.Models;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace generator_web.Controllers
{
    [Authorize]
    public class HomeController : Controller
    {
        private readonly AppDbContext _context;

        public HomeController(AppDbContext context)
        {
            _context = context;
        }

        public async Task<IActionResult> Index()
        {
            // Récupérer les dernières données
            var data = _context.generator_datas
                        .OrderByDescending(d => d.timestamp)
                        .FirstOrDefault();

            // Gérer les alertes
            var currentAlerts = await ProcessAlertsAsync(data);

            ViewBag.Alerts = currentAlerts;
            return View(data);
        }

        private async Task<List<Alert>> ProcessAlertsAsync(generator_data data)
        {
            var currentAlerts = new List<Alert>();

            if (data == null)
                return currentAlerts;   

            try
            {
                // Désactiver toutes les alertes actives
                var activeAlerts = await _context.Alerts
                    .Where(a => a.IsActive)
                    .ToListAsync();

                foreach (var alert in activeAlerts)
                {
                    alert.IsActive = false;
                }

                // Créer de nouvelles alertes selon les conditions
                await CheckAndCreateAlert(data, currentAlerts,
                    condition: data.YakitSeviyesi < 25,
                    type: "FUEL_LOW",
                    message: $"Yakıt seviyesi düşük ({data.YakitSeviyesi}%)");

                await CheckAndCreateAlert(data, currentAlerts,
                    condition: data.SebekeHz == 0 && data.GenUretilenGuc == 0,
                    type: "NO_POWER",
                    message: "Güç kaynağı bulunamadı (ne elektrik ne de jeneratör)");

                await CheckAndCreateAlert(data, currentAlerts,
                    condition: data.SebekeHz > 0 && data.GenUretilenGuc > 0,
                    type: "DUAL_POWER",
                    message: "Anormal durum - Elektrik ve jeneratör aynı anda çalışıyor");

                // Sauvegarder tous les changements
                await _context.SaveChangesAsync();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Erreur lors du traitement des alertes : {ex.Message}");
            }

            return currentAlerts;
        }

        private async Task CheckAndCreateAlert(generator_data data, List<Alert> alertsList,
            bool condition, string type, string message)
        {
            if (condition)
            {
                var alert = new Alert
                {
                    Type = type,
                    Message = message,
                    IsActive = true,
                    CreatedAt = DateTime.Now
                };

                alertsList.Add(alert);
                _context.Alerts.Add(alert);
            }
        }

        public IActionResult Privacy()
        {
            return View();
        }

        [ResponseCache(Duration = 0, Location = ResponseCacheLocation.None, NoStore = true)]
        public IActionResult Error()
        {
            return View(new ErrorViewModel { RequestId = Activity.Current?.Id ?? HttpContext.TraceIdentifier });
        }
    }
}