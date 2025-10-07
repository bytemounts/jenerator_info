using Microsoft.EntityFrameworkCore;
using System.ComponentModel.DataAnnotations;

namespace generator_web.Models
{
    public class User
    {
        [Key]
        public int UserId { get; set; }
        [Required]
        public string Username { get; set; }
        [Required]
        public string email { get; set; }
        [Required]
        public string PasswordHash { get; set; }
    }
}
