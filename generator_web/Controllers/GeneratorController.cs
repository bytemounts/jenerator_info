using generator_web.Models;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Identity;
using Microsoft.AspNetCore.Mvc;
using System.Security.Claims;


namespace generator_web.Controllers
{
    [Authorize]
    [ApiController]
    [Route("api/[controller]")]
    public class GeneratorController : ControllerBase
    {
        private readonly AppDbContext _context;
        private readonly IHttpClientFactory _httpClientFactory;
        private readonly string _espBaseUrl;

        public GeneratorController(AppDbContext context,IHttpClientFactory httpClientFactory, IConfiguration configuration)
        {
            _context = context;
            _httpClientFactory = httpClientFactory;
            _espBaseUrl = configuration.GetValue<string>("Esp:BaseUrl") ?? "http://10.82.134.241:5156";
        }

        [HttpPost("add")]
        [AllowAnonymous]
        public async Task<IActionResult> Add([FromBody] generator_data data)
        {
            try
            {
                // 1. Veriyi kontrol et
                if (data == null)
                    return BadRequest("Veri boş olamaz");

                // 3. Veritabanına ekle
                _context.generator_datas.Add(data);

                // 4. Kaydet
                await _context.SaveChangesAsync();

                // 5. Başarılı cevap dön
                return Ok(new
                {
                    message = "Veri başarıyla eklendi",
                    timestamp = data.timestamp
                });
            }
            catch (Exception ex)
            {
                // Hata durumunda log ve cevap
                return StatusCode(500, new
                {
                    message = "Veri eklenirken hata oluştu",
                    error = ex.Message
                });
            }
        }

        [HttpGet]
        public async Task<IActionResult> GetAll()
        {
            try
            {
                var data = await Task.FromResult(_context.generator_datas.ToList());
                return Ok(data);
            }
            catch (Exception ex)
            {
                return StatusCode(500, new
                {
                    message = "Veri alınırken hata oluştu",
                    error = ex.Message
                });
            }
        }

        [HttpGet("latest")]
        public async Task<IActionResult> GetLatest()
        {
            try
            {
                var latestData = await Task.FromResult(
                    _context.generator_datas
                        .OrderByDescending(x => x.Id)
                        .FirstOrDefault()
                );

                if (latestData == null)
                    return NotFound("Veri bulunamadı");

                return Ok(latestData);
            }
            catch (Exception ex)
            {
                return StatusCode(500, new
                {
                    message = "Veri alınırken hata oluştu",
                    error = ex.Message
                });
            }
        }

        [Authorize]
        [HttpPost("LogControlAction")]
        public async Task<IActionResult> LogControlAction([FromBody] string cmd)
        {
            try
            {
                var userIdClaim = User.FindFirst(ClaimTypes.NameIdentifier);  // ✅ ou "UserId" selon votre choix
                if (userIdClaim == null)
                {
                    return Unauthorized(new { success = false, message = "Unauthorized" });
                }
                int userId = int.Parse(userIdClaim.Value);
                var user = _context.Users.FirstOrDefault(x => x.UserId == userId);
                if(user == null)
                {
                    return BadRequest();
                }

                var controlAction = new ControlAction
                {
                    userName = user.Username,
                    ActionType = cmd,
                    Description = "BUM",
                    Timestamp = DateTime.Now,
                    IpAddress = "ipAddress",
                    IsExecuted = true, // WebSocket'e gönderildiği için executed kabul ediyoruz
                    Status = "SUCCESS",
                    ExecutedAt = DateTime.Now,
                    Result = "Komut ESP32'ye gönderildi"
                };

                _context.ControlActions.Add(controlAction);
                await _context.SaveChangesAsync();

                return Ok(new
                {
                    success = true,
                    message = "Log kaydı başarıyla oluşturuldu",
                    logId = controlAction.Id
                });
            }
            catch (Exception ex)
            {
                return StatusCode(500, new
                {
                    success = false,
                    message = $"Log kaydı oluşturulamadı: {ex.Message}"
                });
            }
        }
    }

}